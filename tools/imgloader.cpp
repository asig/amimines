#include "imgloader.h"

#include <iostream>
#include <cstring>
#include <stdexcept>

#include <png.h>

//
// IffLoader
//

IffLoader::IffLoader(FILE *fp)
: fp_(fp) {
    uint8_t buf[5];
    buf[4] = 0;
    fseek(fp_, 0, SEEK_SET);
    fread(buf, 1, 4, fp_);
    if (std::memcmp("FORM", buf, 4) != 0) {
        throw std::runtime_error("Not an IFF file: bad file magic");
    }

    std::int32_t remainingSize = readUint32();

    fread(buf, 1, 4, fp_); remainingSize-=4;
    if (std::memcmp("ILBM", buf, 4) != 0) {
        throw std::runtime_error("Not an ILBM file");
    }

    while (remainingSize > 0) {
        fread(buf, 1, 4, fp_);
        std::uint32_t sz = readUint32();
        remainingSize -= 8+sz;

        long pos = ftell(fp);
        if (std::memcmp("BMHD", buf, 4) == 0) {
            readBMHD();
        } else if (std::memcmp("CMAP", buf, 4) == 0) {
            readCMAP();
        } else if (std::memcmp("BODY", buf, 4) == 0) {
            readBODY();
        // } else {
        //    std::cout << "Ignoring chunk " << static_cast<unsigned char*>(buf) << std::endl;
        }
        if (sz & 1) {
            // skip padding
            sz++;
            remainingSize--;
        }
        fseek(fp, pos + sz, SEEK_SET);
    }
    //std::cout << "Done reading" << std::endl;
}

IffLoader::~IffLoader() {

}

void IffLoader::readBMHD() {
    w_ = readUint16();
    lineBytes_ = (w_ + 15)/16*2;
    h_ = readUint16();
    skip(4); // Origin
    numPlanes_ = readUint8();
    skip(1); // mask
    compressionMode_  = readUint8();
    if (compressionMode_ > 1) {
        throw std::runtime_error("Unsupported compression scheme");
    }
    skip(1+2+1+1+2+2); // pad, transClr, xAspect, yAspect, pageWidth, pageHeight
}

void IffLoader::readCMAP() {
    palette_.clear();
    for (int i = 0; i < 1 << numPlanes_; i++) {
        uint16_t r = readUint8();
        uint16_t g = readUint8();
        uint16_t b = readUint8();
        palette_.push_back( r/16 << 8 | g/16 << 4 | b/16 );
    }
}

void IffLoader::readBODYUncompressed() {
    planes_.resize(numPlanes_);
    for (int i = 0; i < numPlanes_; i++) {
        planes_[i].reserve(lineBytes_*h_);
    }

    for (int y = 0; y < h_; y++) {
        for (int p = 0; p < numPlanes_; p++) {
            for (int i = 0; i < lineBytes_; i++) {
                std:uint8_t b = readUint8();
                planes_[p].push_back(b);
            }
        }
    }                                        
}

void IffLoader::readBODYRle() {
    // https://web.archive.org/web/20151001160251/https://www.etwright.org/lwsdk/docs/filefmts/ilbm.html
    planes_.resize(numPlanes_);
    for (int i = 0; i < numPlanes_; i++) {
        planes_[i].reserve(lineBytes_*h_);
    }

    for (int y = 0; y < h_; y++) {
        for (int p = 0; p < numPlanes_; p++) {
            std::vector<uint8_t> data;
            int cnt = 0;
            while (cnt < lineBytes_) {
                std:uint8_t b = readUint8();
                if (b > 128) {
                    std::uint8_t v = readUint8();
                    for (int j = 0; j < 257-b; j++) {
                        planes_[p].push_back(v);
                    }
                    cnt += 257-b;
                } else if (b < 128) {
                    for (int j = 0; j < b+1; j++) {
                        std::uint8_t v = readUint8();
                        planes_[p].push_back(v);                                        
                    }
                    cnt += b+1;
                } else {
                    break;
                }
            }
        }
    }
}

void IffLoader::readBODY() {
    switch(compressionMode_) {
        case 0: readBODYUncompressed(); break;
        case 1: readBODYRle(); break;
        default: throw std::runtime_error("Unsupported compression scheme");
    }
}

void IffLoader::skip(long b) {
    fseek(fp_, b, SEEK_CUR);
}

std::uint32_t IffLoader::readUint32() {
    std::uint8_t buf[4];
    fread(buf, 1, 4, fp_);
    return static_cast<uint32_t>(buf[0]) << 24 | static_cast<uint32_t>(buf[1]) << 16 | static_cast<uint32_t>(buf[2]) << 8 | static_cast<uint32_t>(buf[3]);
}

std::uint16_t IffLoader::readUint16() {
    std::uint8_t buf[2];
    fread(buf, 1, 2, fp_);
    return static_cast<uint16_t>(buf[0]) << 8 | static_cast<uint16_t>(buf[1]);
}

std::uint8_t IffLoader::readUint8() {
    return (std::uint8_t)fgetc(fp_);
}

bool IffLoader::canLoad(FILE *fp) {
    static std::uint8_t iffMagic[] = {'F', 'O', 'R', 'M'};
    
    std::uint8_t buf[4];
    fseek(fp, 0, SEEK_SET);
    fread(buf, 1, 4, fp);
    fseek(fp, 0, SEEK_SET);
    return std::memcmp(buf, iffMagic, 4) == 0;
}

//
// PngLoader
//

PngLoader::PngLoader(FILE *fp) {
    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    png_infop info = png_create_info_struct(png);
    if(setjmp(png_jmpbuf(png))) abort();

    png_init_io(png, fp);
    png_read_info(png, info);
    w_ = png_get_image_width(png, info);
    h_ = png_get_image_height(png, info);
    png_byte colorType = png_get_color_type(png, info);
    if (colorType != PNG_COLOR_TYPE_PALETTE) {
        throw std::runtime_error("Unsupported colorType " + std::to_string(colorType) + ", only PNG_COLOR_TYPE_PALETTE is supported");
    }
    numPlanes_ = png_get_bit_depth(png, info);

    // Load image data
    size_t rowBytes = png_get_rowbytes(png,info);
    png_bytepp rows = new png_bytep[h_];
    for(int y = 0; y < h_; y++) {
        rows[y] = new png_byte[rowBytes];
    }
    png_read_image(png, rows);    

    
    // transform image to 8bit if necessary
    std::vector<std::uint8_t> image;
    image.reserve(w_ * h_);
    switch(numPlanes_) {
    case 4:
        // Turn 4 bit/pixel to 8 bit/pixel
        for (int y = 0; y < h_; y++) {
            int x = 0;
            for (int pngX = 0; pngX < rowBytes; pngX++) {
                std::uint8_t val = rows[y][pngX];
                image[y * w_ + x++] = val >> 4;
                image[y * w_ + x++] = val & 0xf;
            }
        }
        break;
    case 8:
        for (int y = 0; y < h_; y++) {
            for (int x = 0; x < rowBytes; x++) {
                std::uint8_t val = rows[y][x];
            }
        }
        break;
    default:
        throw std::runtime_error("Unsupported bit depth " + std::to_string(numPlanes_));
    }
    
    // separate into planes
    planes_.reserve(numPlanes_);
    lineBytes_ = ((w_+15)/16) * 2; // padded width in bytes;
    for(int i = 0; i < numPlanes_; i++) {
        std::uint8_t mask = 1 << i;
        std::vector<std::uint8_t> data;
        data.resize(lineBytes_ * h_);
        for (int y = 0; y < h_; y++) {
            for (int b = 0; b < lineBytes_; b++) {
                std::uint8_t val = 0;
                for (int x = 0; x < 8; x++) {
                    int iX = b * 8 + x;
                    if (iX >= w_) continue;
                    uint8_t col = image[y*w_ + iX];                    
                    if (col & mask) {
                        val |= 1 << (7-x);
                    }
                }
                data[y*lineBytes_ + b] = val;
            }
        }
        planes_.push_back(data);
    }

    // extract palette
    palette_.clear();
    int numPalette;
    png_colorp palette;
    png_get_PLTE(png, info, &palette, &numPalette);
    palette_.reserve(numPalette);

    for (int i = 0; i < numPalette; i++) {
        std::uint16_t r = palette[i].red/16;
        std::uint16_t g = palette[i].green/16;
        std::uint16_t b = palette[i].blue/16;
        palette_.push_back(r << 8 | g << 4 | b);
    }
}

PngLoader::~PngLoader() {
}

bool PngLoader::canLoad(FILE *fp) {
    static std::uint8_t pngMagic[] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};
    
    std::uint8_t buf[8];
    fseek(fp, 0, SEEK_SET);
    fread(buf, 1, 8, fp);
    fseek(fp, 0, SEEK_SET);
    return std::memcmp(buf, pngMagic, 8) == 0;
}

//
// ImageLoader
//

const std::vector<std::uint8_t>& ImageLoader::image() {
    if (imageComputed_) {
        return image_;
    }

    imageComputed_ = true;

    image_.resize(w_ * h_);

    for (int y = 0; y < h_; y++) {
        for (int x = 0; x < w_; x++) {
            std::uint8_t val = 0;
            for (int plane = 0; plane < numPlanes_; plane++) {
                int bytePos = x/8;
                int bitPos = 7 - x%8;                
                if (planes_[plane][y*lineBytes_ + bytePos] & (1 << bitPos)) {
                    val |= 1 << plane;
                }
            }
            image_[y*w_+x] = val;
        }
    }
    return image_;
}

//
//
//

ImageLoader *getLoader(FILE *fp) {
    if (IffLoader::canLoad(fp)) {
        return new IffLoader(fp);
    }
    if (PngLoader::canLoad(fp)) {
        return new PngLoader(fp);
    }
    return nullptr;
}
