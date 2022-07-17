#include <cmath>
#include <limits>
#include <numeric>
#include <wav_writer.hpp>
int main() {
    wav_writer::WavWriter writer("test.wav");
    decltype(writer)::PCMData samples;
    auto amplitude = std::numeric_limits<decltype(samples)::value_type>::max();
    for (int i = 0; i < 44100; i++) {
        auto sample = amplitude * std::sin(440 * (i / 44100.0) * 2 * std::numbers::pi);
        samples.push_back(sample);
    }
    writer.add_samples(samples);
}
