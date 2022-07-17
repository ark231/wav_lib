#include <fmt/format.h>

#include <array>
#include <bit>
#include <concepts>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <type_traits>
#include <vector>
namespace wav_writer {
enum class FormatTag : std::uint16_t {
    WAVE_FORMAT_PCM = 0x0001,
    WAVE_FORMAT_IEEE_FLOAT = 0x0003,
};
template <FormatTag format, int bitwidth, int num_channels>
struct PCMSample {
    using type = void;
};

template <>
struct PCMSample<FormatTag::WAVE_FORMAT_PCM, 16, 1> {
    using type = std::int16_t;
};
template <int num_channels>
struct PCMSample<FormatTag::WAVE_FORMAT_PCM, 16, num_channels> {
    using type = std::array<std::int16_t, num_channels>;
};
template <>
struct PCMSample<FormatTag::WAVE_FORMAT_IEEE_FLOAT, 32, 1> {
    using type = float;
};
template <int num_channels>
struct PCMSample<FormatTag::WAVE_FORMAT_IEEE_FLOAT, 32, num_channels> {
    using type = std::array<float, num_channels>;
};
namespace {

enum class Endian { little, big };
template <int size, std::integral T>
std::array<char, size> to_bytearray(T value, Endian endian = Endian::little) {
    std::invoke_result_t<decltype(to_bytearray<size, T>), T, Endian> result;
    for (auto i = 0; i < size; i++) {
        switch (endian) {
            case Endian::little:
                result[i] = std::bit_cast<char>(static_cast<std::uint8_t>((value >> (8 * i)) & 0xff));
                break;
            case Endian::big:
                result[i] = std::bit_cast<char>(static_cast<std::uint8_t>((value >> (8 * (size - i - 1))) & 0xff));
                break;
        }
    }
    return result;
}
}  // namespace
template <FormatTag format = FormatTag::WAVE_FORMAT_PCM, std::uint16_t bitwidth = 16, std::uint16_t num_channels = 1>
class WavWriter {
    std::uint16_t format_tag_;
    std::uint16_t num_channels_;
    std::uint32_t num_samples_per_sec_;
    std::uint32_t avg_byte_per_sec_;
    std::uint16_t block_align_;
    std::uint16_t num_bits_per_sample_;
    std::uint16_t cbsize_;  // currently, not used as this doesn't support extension

    std::ofstream outfile_;
    std::uint32_t format_chunk_size_;
    std::uint32_t data_chunk_size_;

    std::ofstream::pos_type riff_chunk_size_pos_;
    std::ofstream::pos_type data_chunk_size_pos_;

   public:
    WavWriter(std::filesystem::path outfilepath, std::uint32_t num_samples_per_sec = 44100)
        : format_tag_(static_cast<std::uint16_t>(format)),
          num_channels_(num_channels),
          num_samples_per_sec_(num_samples_per_sec),
          outfile_(outfilepath, std::ios_base::binary) {
        block_align_ = (num_channels * bitwidth) / 8;
        avg_byte_per_sec_ = num_samples_per_sec * block_align_;
        num_bits_per_sample_ = bitwidth;
        cbsize_ = 0;
        if (not outfile_) {
            throw std::runtime_error(fmt::format("error: couldn't open file {}", outfilepath.string()));
        }
        outfile_.write("RIFF", 4);
        riff_chunk_size_pos_ = outfile_.tellp();
        outfile_.seekp(4, std::ios_base::cur);  // write later
        outfile_.write("WAVE", 4);
        outfile_.write("fmt ", 4);
        format_chunk_size_ = 16;
        outfile_.write(to_bytearray<4>(format_chunk_size_).data(), 4);
        outfile_.write(to_bytearray<2>(format_tag_).data(), 2);
        outfile_.write(to_bytearray<2>(num_channels_).data(), 2);
        outfile_.write(to_bytearray<4>(num_samples_per_sec_).data(), 4);
        outfile_.write(to_bytearray<4>(avg_byte_per_sec_).data(), 4);
        outfile_.write(to_bytearray<2>(block_align_).data(), 2);
        outfile_.write(to_bytearray<2>(num_bits_per_sample_).data(), 2);
        outfile_.write("data", 4);
        data_chunk_size_ = 0;
        data_chunk_size_pos_ = outfile_.tellp();
        outfile_.seekp(4, std::ios_base::cur);  // write later
    }
    ~WavWriter() {
        outfile_.seekp(riff_chunk_size_pos_);
        std::uint32_t riff_chunk_size = sizeof("WAVE") - 1 + sizeof("fmt ") - 1 + sizeof(format_chunk_size_) +
                                        format_chunk_size_ + sizeof("data") - 1 + sizeof(data_chunk_size_) +
                                        data_chunk_size_;
        outfile_.write(to_bytearray<4>(riff_chunk_size).data(), 4);
        outfile_.seekp(data_chunk_size_pos_);
        outfile_.write(to_bytearray<4>(data_chunk_size_).data(), 4);
        outfile_.close();
    }
    using PCMData = std::vector<typename PCMSample<format, bitwidth, num_channels>::type>;
    void add_samples(const PCMData& samples) {
        for (const auto& sample : samples) {
            this->add_sample(sample);
        }
    }

    void add_sample(const typename PCMData::value_type sample) {
        if constexpr (num_channels == 1) {
            constexpr auto SIZE_CHANNEL_SAMPLE = sizeof(decltype(sample));
            data_chunk_size_ += SIZE_CHANNEL_SAMPLE * 1;
            outfile_.write(to_bytearray<SIZE_CHANNEL_SAMPLE>(sample).data(), SIZE_CHANNEL_SAMPLE);
        } else {
            constexpr auto SIZE_CHANNEL_SAMPLE = sizeof(decltype(sample)::value_type);
            data_chunk_size_ += SIZE_CHANNEL_SAMPLE * std::tuple_size_v<decltype(sample)>;
            for (const auto& channel_sample : sample) {
                outfile_.write(to_bytearray<SIZE_CHANNEL_SAMPLE>(channel_sample).data(), SIZE_CHANNEL_SAMPLE);
            }
        }
    }
};

}  // namespace wav_writer
