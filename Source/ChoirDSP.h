#pragma once
#include <JuceHeader.h>
#include <array>
#include <cmath>
#include <vector>

class PitchTracker
{
public:
    void prepare (double newSampleRate)
    {
        sampleRate = newSampleRate;
        ring.assign (4096, 0.0f);
        writePos = 0;
        samplesUntilAnalysis = 0;
        lastMidi = 60;
    }

    int processSample (float x)
    {
        ring[(size_t) writePos] = x;
        writePos = (writePos + 1) % (int) ring.size();

        if (--samplesUntilAnalysis <= 0)
        {
            samplesUntilAnalysis = 768;
            analyse();
        }
        return lastMidi;
    }

private:
    void analyse()
    {
        constexpr int n = 2048;
        std::array<float, n> frame {};
        double mean = 0.0;
        for (int i = 0; i < n; ++i)
        {
            int idx = writePos - n + i;
            while (idx < 0) idx += (int) ring.size();
            frame[(size_t) i] = ring[(size_t) idx];
            mean += frame[(size_t) i];
        }
        mean /= n;

        double energy = 0.0;
        for (auto& v : frame)
        {
            v -= (float) mean;
            energy += (double) v * v;
        }
        if (std::sqrt (energy / n) < 0.004)
            return;

        const int minLag = juce::jmax (16, (int) (sampleRate / 900.0));
        const int maxLag = juce::jmin (n / 2, (int) (sampleRate / 70.0));
        float bestCorr = 0.0f;
        int bestLag = 0;

        for (int lag = minLag; lag <= maxLag; ++lag)
        {
            double sum = 0.0, e1 = 1.0e-12, e2 = 1.0e-12;
            const int count = n - lag;
            for (int i = 0; i < count; i += 2)
            {
                const float a = frame[(size_t) i];
                const float b = frame[(size_t) (i + lag)];
                sum += (double) a * b;
                e1 += (double) a * a;
                e2 += (double) b * b;
            }
            const float corr = (float) (sum / std::sqrt (e1 * e2));
            if (corr > bestCorr)
            {
                bestCorr = corr;
                bestLag = lag;
            }
        }

        if (bestLag > 0 && bestCorr > 0.52f)
        {
            const double hz = sampleRate / bestLag;
            const double midi = 69.0 + 12.0 * std::log2 (hz / 440.0);
            const int candidate = juce::jlimit (24, 96, (int) std::lround (midi));
            // A small amount of hysteresis prevents harmony chatter.
            if (std::abs (candidate - lastMidi) <= 7 || bestCorr > 0.68f)
                lastMidi = candidate;
        }
    }

    double sampleRate = 48000.0;
    std::vector<float> ring;
    int writePos = 0;
    int samplesUntilAnalysis = 0;
    int lastMidi = 60;
};

class GranularPitchVoice
{
public:
    void prepare (double sr)
    {
        sampleRate = sr;
        const int desired = (int) std::ceil (sampleRate * 0.75);
        bufferSize = 1;
        while (bufferSize < desired) bufferSize <<= 1;
        mask = bufferSize - 1;
        buffer.assign ((size_t) bufferSize, 0.0f);
        writePos = 0;

        grainLength = juce::jlimit (1024, 8192, (int) (sampleRate * 0.085));
        baseDelay = grainLength * 2 + (int) (sampleRate * 0.055);
        restartGrain (a, 0.0, 1.0);
        restartGrain (b, grainLength * 0.5, 1.0);
        b.age = grainLength / 2;
    }

    void reset()
    {
        std::fill (buffer.begin(), buffer.end(), 0.0f);
        writePos = 0;
        restartGrain (a, 0.0, 1.0);
        restartGrain (b, grainLength * 0.5, 1.0);
        b.age = grainLength / 2;
    }

    float process (float input, float semitones, float extraDelayMs)
    {
        buffer[(size_t) writePos] = input;
        const double ratio = std::pow (2.0, (double) semitones / 12.0);
        const double extraDelay = sampleRate * extraDelayMs / 1000.0;

        if (a.age >= grainLength) restartGrain (a, extraDelay, ratio);
        if (b.age >= grainLength) restartGrain (b, extraDelay, ratio);

        const float ya = readInterpolated (a.readPos) * window (a.age);
        const float yb = readInterpolated (b.readPos) * window (b.age);

        a.readPos += ratio;
        b.readPos += ratio;
        wrap (a.readPos);
        wrap (b.readPos);
        ++a.age;
        ++b.age;

        writePos = (writePos + 1) & mask;
        return ya + yb;
    }

    int getLatencySamples() const noexcept { return baseDelay; }

private:
    struct Grain { double readPos = 0.0; int age = 0; };

    void restartGrain (Grain& g, double extraDelay, double ratio)
    {
        g.age = 0;
        // Enough look-back for upward transposition without catching the write head.
        g.readPos = (double) writePos - (double) baseDelay - extraDelay;
        // Small correction keeps the effective centre of differently pitched grains aligned.
        g.readPos -= 0.25 * grainLength * juce::jmax (0.0, ratio - 1.0);
        wrap (g.readPos);
    }

    float readInterpolated (double pos) const
    {
        int i0 = (int) std::floor (pos) & mask;
        int i1 = (i0 + 1) & mask;
        const float frac = (float) (pos - std::floor (pos));
        return buffer[(size_t) i0] + frac * (buffer[(size_t) i1] - buffer[(size_t) i0]);
    }

    float window (int age) const
    {
        const float p = juce::jlimit (0.0f, 1.0f, (float) age / (float) grainLength);
        const float s = std::sin (juce::MathConstants<float>::pi * p);
        return s * s;
    }

    void wrap (double& p) const
    {
        while (p < 0.0) p += bufferSize;
        while (p >= bufferSize) p -= bufferSize;
    }

    double sampleRate = 48000.0;
    int bufferSize = 0, mask = 0, writePos = 0;
    int grainLength = 4096, baseDelay = 8192;
    std::vector<float> buffer;
    Grain a, b;
};

class SimpleDelay
{
public:
    void prepare (int maximumSamples)
    {
        size = 1;
        while (size < maximumSamples + 8) size <<= 1;
        mask = size - 1;
        data.assign ((size_t) size, 0.0f);
        w = 0;
    }

    float process (float x, int delaySamples)
    {
        data[(size_t) w] = x;
        const int r = (w - juce::jlimit (0, size - 2, delaySamples)) & mask;
        const float y = data[(size_t) r];
        w = (w + 1) & mask;
        return y;
    }

private:
    std::vector<float> data;
    int size = 0, mask = 0, w = 0;
};

inline int floorDiv7 (int x)
{
    int q = x / 7;
    const int r = x % 7;
    if (r < 0) --q;
    return q;
}

inline int positiveMod7 (int x)
{
    int r = x % 7;
    return r < 0 ? r + 7 : r;
}

inline int diatonicSemitoneOffset (int midiNote, int rootPc, bool minor, int degreeSteps)
{
    static constexpr int majorScale[7] = { 0, 2, 4, 5, 7, 9, 11 };
    static constexpr int minorScale[7] = { 0, 2, 3, 5, 7, 8, 10 };
    const int* scale = minor ? minorScale : majorScale;

    const int rel = midiNote - rootPc;
    const int approxOct = (int) std::floor ((double) rel / 12.0);
    int bestDegreeGlobal = approxOct * 7;
    int bestDistance = 999;

    for (int oct = approxOct - 1; oct <= approxOct + 1; ++oct)
    {
        for (int d = 0; d < 7; ++d)
        {
            const int candidate = oct * 12 + scale[d];
            const int dist = std::abs (candidate - rel);
            if (dist < bestDistance)
            {
                bestDistance = dist;
                bestDegreeGlobal = oct * 7 + d;
            }
        }
    }

    const int targetGlobal = bestDegreeGlobal + degreeSteps;
    const int targetOct = floorDiv7 (targetGlobal);
    const int targetDegree = positiveMod7 (targetGlobal);
    const int targetRel = targetOct * 12 + scale[targetDegree];
    return targetRel - rel;
}
