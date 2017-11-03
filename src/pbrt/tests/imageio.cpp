
#include <gtest/gtest.h>
#include <pbrt/core/pbrt.h>
#include <pbrt/core/image.h>
#include <pbrt/util/fileutil.h>
#include <pbrt/core/spectrum.h>

using namespace pbrt;

static std::string inTestDir(const std::string &path) { return path; }

static void TestRoundTrip(const char *fn, bool gamma) {
    Point2i res(16, 29);
    Image image(PixelFormat::RGB32, res);
    for (int y = 0; y < res[1]; ++y)
        for (int x = 0; x < res[0]; ++x) {
            Float rgb[3] = {Float(x) / Float(res[0] - 1),
                            Float(y) / Float(res[1] - 1), Float(-1.5)};
            Spectrum s = Spectrum::FromRGB(rgb);
            image.SetSpectrum({x, y}, s);
        }

    std::string filename = inTestDir(fn);
    ASSERT_TRUE(image.Write(filename));

    Point2i readRes;
    auto readImage = Image::Read(filename);
    ASSERT_TRUE((bool)readImage);
    ASSERT_EQ(readImage->resolution, res);

    for (int y = 0; y < res[1]; ++y)
        for (int x = 0; x < res[0]; ++x) {
            Spectrum s = readImage->GetSpectrum({x, y});
            std::array<Float, 3> rgb = s.ToRGB();

            for (int c = 0; c < 3; ++c) {
                float wrote = image.GetChannel({x, y}, c);
                float delta = wrote - rgb[c];
                if (HasExtension(filename, "pfm")) {
                    // Everything should come out exact.
                    EXPECT_EQ(0, delta) << filename << ":(" << x << ", " << y
                                        << ") c = " << c << " wrote " << wrote
                                        << ", read " << rgb[c]
                                        << ", delta = " << delta;
                } else if (HasExtension(filename, "exr")) {
                    if (c == 2)
                        // -1.5 is exactly representable as a float.
                        EXPECT_EQ(0, delta) << "(" << x << ", " << y
                                            << ") c = " << c << " wrote "
                                            << wrote << ", read " << rgb[c]
                                            << ", delta = " << delta;
                    else
                        EXPECT_LT(std::abs(delta), .001)
                            << filename << ":(" << x << ", " << y << ") c = " << c
                            << " wrote " << wrote << ", read " << rgb[c]
                            << ", delta = " << delta;
                } else {
                    // 8 bit format...
                    if (c == 2)
                        // -1.5 should be clamped to zero.
                        EXPECT_EQ(0, rgb[c]) << "(" << x << ", " << y
                                             << ") c = " << c << " wrote "
                                             << wrote << ", read " << rgb[c]
                                             << " (expected 0 back)";
                    else
                        // Allow a fair amount of slop, since there's an sRGB
                        // conversion before quantization to 8-bits...
                        EXPECT_LT(std::abs(delta), .02)
                            << filename << ":(" << x << ", " << y << ") c = " << c
                            << " wrote " << wrote << ", read " << rgb[c]
                            << ", delta = " << delta;
                }
            }
        }

    // Clean up
    EXPECT_EQ(0, remove(filename.c_str()));
}

TEST(ImageIO, RoundTripEXR) { TestRoundTrip("out.exr", false); }

TEST(ImageIO, RoundTripPFM) { TestRoundTrip("out.pfm", false); }

TEST(ImageIO, RoundTripPNG) { TestRoundTrip("out.png", true); }
