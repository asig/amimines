#include "imgloader.h"

#include <iostream>
#include <cstring>
#include <stdexcept>

#include <png.h>

//
// Image
//

Image::Image() 
: w_(0), h_(0), depth_(0), lineBytes_(0) {
}

Image::Image(int w, int h, int depth, int lineBytes, const std::vector<std::uint8_t>& data)
: w_(w), h_(h), depth_(depth), lineBytes_(lineBytes), data_(data) {
    // separate into planes
    planes_.reserve(depth_);
    lineBytes_ = ((w_+15)/16) * 2; // padded width in bytes;
    for(int i = 0; i < depth_; i++) {
        std::uint8_t mask = 1 << i;
        std::vector<std::uint16_t> planeData;
        planeData.resize(lineBytes_/2 * h_);
        for (int y = 0; y < h_; y++) {
            for (int w = 0; w < lineBytes_/2; w++) {
                std::uint16_t val = 0;
                for (int x = 0; x < 16; x++) {
                    int iX = w * 16 + x;
                    if (iX >= w_) continue;
                    uint8_t col = data_[y*w_ + iX];                    
                    if (col & mask) {
                        val |= 1 << (15-x);
                    }
                }
                planeData[y*lineBytes_/2 + w] = val;
            }
        }
        planes_.push_back(planeData);
    }
}

Image::Image(int w, int h, int depth, int lineBytes, const std::vector<std::vector<std::uint16_t> >& planes)
: w_(w), h_(h), depth_(depth), lineBytes_(lineBytes), planes_(planes) {
    // merge planes into image
    data_.clear();
    data_.resize(h_ * w_);
    for (int y = 0; y < h_; y++) {
        for (int x = 0; x < w_; x++) {
            std::uint8_t val = 0;
            for (int plane = 0; plane < depth_; plane++) {
                int wordPos = x/16;
                int bitPos = 15 - x%16;                
                if (planes_[plane][y*lineBytes_/2 + wordPos] & (1 << bitPos)) {
                    val |= 1 << plane;
                }
            }
            data_[y*w_+x] = val;
        }
    }
}

Image Image::extract(int x, int y, int w, int h) const {
    int lineBytes = (w+15)/16 * 2;
    std::vector<std::uint8_t> data;
    data.resize(h * w);

    for (int yy = 0; yy < h; yy++) {
        for (int xx = 0; xx < w; xx++) {
            data[yy * w + xx] = data_[(y+yy) * w_ + (x + xx)];
        }
    }
    return Image(w,h,depth_,lineBytes,data);
}

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
    image_ = Image(w_, h_, numPlanes_, lineBytes_, planes_);
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
        planes_[i].reserve(lineBytes_/2*h_);
    }

    for (int y = 0; y < h_; y++) {
        for (int p = 0; p < numPlanes_; p++) {
            for (int i = 0; i < lineBytes_; i+=2) {
                std::uint16_t b1 = readUint8();
                std::uint16_t b2 = readUint8();
                planes_[p].push_back(b1 << 8 | b2);
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
            std::vector<std::uint8_t> line;
            while (cnt < lineBytes_) {
                std:uint8_t b = readUint8();
                if (b > 128) {
                    std::uint8_t v = readUint8();
                    for (int j = 0; j < 257-b; j++) {
                        line.push_back(v);
                    }
                    cnt += 257-b;
                } else if (b < 128) {
                    for (int j = 0; j < b+1; j++) {
                        std::uint8_t v = readUint8();
                        line.push_back(v);                                        
                    }
                    cnt += b+1;
                } else {
                    break;
                }
            }
            for(int i = 0; i < line.size(); i+=2) {
                std::uint16_t b1 = line[i];
                std::uint16_t b2 = line[i+1];
                planes_[p].push_back(b1 << 8 | b2);
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
    int w = png_get_image_width(png, info);
    int h = png_get_image_height(png, info);
    png_byte colorType = png_get_color_type(png, info);
    if (colorType != PNG_COLOR_TYPE_PALETTE) {
        throw std::runtime_error("Unsupported colorType " + std::to_string(colorType) + ", only PNG_COLOR_TYPE_PALETTE is supported");
    }
    int depth = png_get_bit_depth(png, info);

    // Load image data
    size_t rowBytes = png_get_rowbytes(png,info);
    png_bytepp rows = new png_bytep[h];
    for(int y = 0; y < h; y++) {
        rows[y] = new png_byte[rowBytes];
    }
    png_read_image(png, rows);    

    
    // transform image to 8bit if necessary
    std::vector<std::uint8_t> image;
    int newRowBytes = w;
    if (w % 2) {
        // needs padding!
        newRowBytes++;
    }
    image.reserve(newRowBytes * h);
    switch(depth) {
    case 4:
        // Turn 4 bit/pixel to 8 bit/pixel
        for (int y = 0; y < h; y++) {
            int x = 0;
            for (int pngX = 0; pngX < rowBytes; pngX++) {
                std::uint8_t val = rows[y][pngX];
                image[y * w + x++] = val >> 4;
                image[y * w + x++] = val & 0xf;
            }
        }
        break;
    case 8:
        for (int y = 0; y < h; y++) {
            for (int x = 0; x < rowBytes; x++) {
                std::uint8_t val = rows[y][x];
            }
        }
        break;
    default:
        throw std::runtime_error("Unsupported bit depth " + std::to_string(depth));
    }
    
    image_ = Image(w, h, depth, newRowBytes, image);

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

ImageLoader *ImageLoader::get(FILE *fp) {
    if (IffLoader::canLoad(fp)) {
        return new IffLoader(fp);
    }
    if (PngLoader::canLoad(fp)) {
        return new PngLoader(fp);
    }
    return nullptr;
}
