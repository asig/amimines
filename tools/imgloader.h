#pragma once

#include <stdio.h>

#include <vector>
#include <cstdint>

class ImageLoader {
public:
    ImageLoader() :
        imageComputed_(false) {}

    virtual ~ImageLoader() {}
    
    virtual int numPlanes() const {
        return numPlanes_;
    }

    virtual const std::vector<std::uint16_t>& palette() {
        return palette_;
    }

    virtual const std::vector<std::uint8_t>& bitplane(int plane) {
        return planes_[plane];
    }

    virtual int w() const {
        return w_;
    }

    virtual int h() const {
        return h_;
    }

    virtual const std::vector<std::uint8_t>& image();

protected:
    std::uint16_t w_, h_;
    std::uint8_t numPlanes_;
    std::uint16_t lineBytes_;
    std::vector<std::uint16_t> palette_;
    std::vector<std::vector<std::uint8_t> > planes_;

private:
    bool imageComputed_;
    std::vector<std::uint8_t> image_;

};

class PngLoader : public ImageLoader {
public:
    explicit PngLoader(FILE *fp);
    virtual ~PngLoader();

    static bool canLoad(FILE *fp);
};

class IffLoader : public ImageLoader {
public:
    explicit IffLoader(FILE *fp);
    virtual ~IffLoader();

    static bool canLoad(FILE *fp);

private:
    FILE *fp_;

    std::uint8_t compressionMode_;

    void readBMHD();
    void readCMAP();
    void readBODY();
    void readBODYUncompressed();
    void readBODYRle();

    void skip(long b);

    std::uint32_t readUint32();
    std::uint16_t readUint16();
    std::uint8_t readUint8();
};

ImageLoader *getLoader(FILE *fp);
