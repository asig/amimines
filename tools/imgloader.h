#pragma once

#include <stdio.h>

#include <vector>
#include <cstdint>


class Image {
public:
    Image();
    Image(int w, int h, int depth, int lineBytes, const std::vector<std::uint8_t>& data);
    Image(int w, int h, int depth, int lineBytes, const std::vector<std::vector<std::uint16_t> >& planes);

    Image extract(int x, int y, int w, int h) const;

    const std::vector<std::uint16_t>& bitplane(int p) const {
        return planes_[p];
    }

    const std::vector<std::uint8_t>& data() const {
        return data_;
    }

    int w() const {
        return w_;
    }

    int h() const {
        return h_;
    }

    int lineBytes() const {
        return lineBytes_;
    }

    int depth() const {
        return depth_;
    }

private:
    int w_;
    int h_;
    int depth_;
    int lineBytes_;
    std::vector<std::uint8_t> data_;
    std::vector<std::vector<std::uint16_t> > planes_;
};


class ImageLoader {
public:
    ImageLoader() {}
    virtual ~ImageLoader() { }
    
    virtual const std::vector<std::uint16_t>& palette() {
        return palette_;
    }

    virtual Image image() const {
        return image_;
    }

    static ImageLoader *get(FILE *fp);

protected:
    Image image_;
    std::vector<std::uint16_t> palette_;
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
    int lineBytes_;
    int w_;
    int h_;
    int numPlanes_;
    std::uint8_t compressionMode_;
    std::vector<std::vector<std::uint16_t> > planes_;

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
