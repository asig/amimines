// Format taken from http://www.evillabs.net/index.php/Amiga_Icon_Formats, with
// some adjustments.

#include <cstdint>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <vector>
#include <sstream>
#include <initializer_list>

#include <getopt.h>

#include "imgloader.h"

enum class Type {
    UNDEFINED = 0,
    DISK = 1,     // a disk
    DRAWER = 2,   // a directory
    TOOL = 3,     // a program
    PROJECT = 4,  // a project file with defined program to start
    GARBAGE = 5,  // the trashcan
    DEVICE = 6,   // should never appear
    KICK = 7,     // a kickstart disk
    APPICON = 8,  // should never appear
};

std::unordered_map<std::string, Type> typeNameToVal = {
    {"DISK", Type::DISK},
    {"DRAWER", Type::DRAWER},
    {"TOOL", Type::TOOL},
    {"PROJECT", Type::PROJECT},
    {"GARBAGE", Type::GARBAGE},
    {"DEVICE", Type::DEVICE},
    {"KICK", Type::KICK},
    {"APPICON", Type::APPICON},
};

struct Region {
    long x, y, w, h;
};

class binstream {
public: 
    binstream(std::ostream &os) : os_(os), pos_(0) {}

    void checkPos(std::uint32_t pos) {
        if (pos_ != pos) {
            std::cerr << "BAD POS: Expected " << pos << ", actual " << pos_ << std::endl;
            abort();
        }
    }

    binstream &operator<<(std::uint32_t v) {
        os_
            << (std::uint8_t)(v>>24 & 0xff) 
            << (std::uint8_t)(v>>16 & 0xff) 
            << (std::uint8_t)(v>>8 & 0xff)
            << (std::uint8_t)(v & 0xff);
        pos_ += 4;
        return *this;
    }

    binstream &operator<<(std::int32_t v) {        
        os_
            << (std::uint8_t)(v>>24 & 0xff) 
            << (std::uint8_t)(v>>16 & 0xff) 
            << (std::uint8_t)(v>>8 & 0xff)
            << (std::uint8_t)(v & 0xff);
        pos_ += 4;
        return *this;
    }

    binstream &operator<<(std::uint16_t v) {
        os_
            << (std::uint8_t)(v>>8 & 0xff)
            << (std::uint8_t)(v & 0xff);
        pos_ += 2;
        return *this;
    }

    binstream &operator<<(std::int16_t v) {        
        os_
            << (std::uint8_t)(v>>8 & 0xff)
            << (std::uint8_t)(v & 0xff);
        pos_ += 2;
        return *this;
    }

    binstream &operator<<(std::uint8_t v) {        
        os_ << (std::uint8_t)v;
        pos_++;;
        return *this;
    }

    binstream &operator<<(std::int8_t v) {        
        os_ << (std::uint8_t)v;
        pos_++;;
        return *this;
    }

    binstream &operator<<(char v) {
        os_ << (std::uint8_t)v;
        pos_++;;
        return *this;
    }

private:
    std::ostream &os_;    
    std::uint32_t pos_;
};

void usage() {
    std::cerr 
        << "usage: infogen --type <type> [--stacksize <stacksize>] {--icon path/to/icon@<region>} [--x int] [--y int] info-file-name" << "\n"
        << "   where\n"
        << "      <type> is one of DISK, DRAWER, TOOL, PROJECT, GARBAGE, DEVICE, KICK, or APPICON" << "\n"
        << "      <stacksize> is an integer > 4096" << "\n"
        << "   --icon needs to be present at least once, and at most 2 times." << "\n"
        << "      <region> specifies the part of the image that should be used, and is expressed as x,y,w,h ." << "\n"
        << "      Example: to extract 3232 pixels at the position (100,150) from foo.iff, use: --icon foo.iff@100,150,32,32\n";
    exit(1);
}

bool parseInt(const char* s, long& val) {
    if (!s) {
        return false;
    }
    char *endptr;
    val = strtol(s, &endptr, 10);
    return (*endptr == 0 || isspace(*endptr));
}

bool parseUint(char* s, long& val) {
    return parseInt(s, val) && val >= 0;
}

bool parsePathWithRegion(const char* raw, std::string& path, Region& region) {    
    region.x = region.y = region.w = region.h = -1;

    if (!raw) {
        return false;
    }

    path = raw;
    std::size_t sepPos = path.find('@');
    if (sepPos == std::string::npos) {
        return true;
    }

    // split path and region
    std::string regionString = path.substr(sepPos+1);
    path = path.substr(0, sepPos);

    // parse region
    std::vector<std::string> regionParts;
    std::istringstream is(regionString);
    std::string s;    
    while (getline(is, s, ',')) {
        regionParts.push_back(s);
    }
    if (regionParts.size() != 4) {
        return false;
    }
    if (!parseInt(regionParts[0].c_str(), region.x)) {
        return false;
    }
    if (!parseInt(regionParts[1].c_str(), region.y)) {
        return false;
    }
    if (!parseInt(regionParts[2].c_str(), region.w)) {
        return false;
    }
    if (!parseInt(regionParts[3].c_str(), region.h)) {
        return false;
    }
    return true;
}



void addImage(const Image& img, binstream& os) {
    // 0x00 WORD  im_LeftEdge       always 0 ???
    os << (std::int16_t)0;

    // 0x00 WORD  im_TopEdge        always 0 ???
    os << (std::int16_t)0;

    // 0x04 WORD  im_Width          the width of the image
    os << (std::int16_t)img.w();

    // 0x06 WORD  im_Height         the height of the image
    os << (std::int16_t)img.h();

    // 0x08 WORD  im_Depth          the image bitmap depth
    os << (std::int16_t)img.depth();

    // 0x0A APTR  im_ImageData      <boolean> always true ???
    os << (std::int32_t)0x1234; // byte order doesn't matter.

    // 0x0E UBYTE im_PlanePick      foreground color register index
    os << (std::uint8_t)((1<<img.depth())-1); // select every plane

    // 0x0F UBYTE im_PlaneOnOff     background color register index
    os << (std::uint8_t)0; // select no plane

    // 0x10 APTR  im_Next           always 0 ???
    os << (std::int32_t)0;

    for (int i = 0; i < img.depth(); i++) {
        const std::vector<std::uint16_t>& plane = img.bitplane(i);
        for (int i = 0; i < plane.size(); i++) {
            os << (std::uint8_t)(plane[i] >> 8);
            os << (std::uint8_t)(plane[i] & 0xff);
        }
    }
}

int main(int argc, char *const *argv) {

    Type type = Type::UNDEFINED;
    std::int32_t stackSize = 4096;
    std::string filename;
    std::vector<Image> images;
    int posX = 50;
    int posY = 50;

    static option longopts[] = {
        {"type", required_argument, nullptr, 't'},
        {"stacksize", required_argument, nullptr, 's'},
        {"icon", required_argument, nullptr, 'i'},
        {"x", required_argument, nullptr, 'x'},
        {"y", required_argument, nullptr, 'y'},
        {0,0,0,0},
    };
    for(;;) {
        int option_index = 0;
        int c = getopt_long (argc, argv, "t:sixy", longopts, &option_index);
        if (c == -1) {
            // end of options
            break;
        }
        switch(c) {
            case 't':
                {
                    auto it = typeNameToVal.find(optarg);
                    if (it == typeNameToVal.end()) {
                        std::cerr << "Unrecognized type " << optarg << "\n";
                        usage();
                    }
                    type = it->second;
                }
                break;
            case 's':
                {
                    long int val;
                    if (!parseUint(optarg, val)) {
                        std::cerr << "Invalid stack size " << optarg << "\n";
                        usage();
                    }
                    if (val < 4096) {
                        std::cerr << "Stack size " << val << " too small, using 4096 instead." << "\n";
                        val = 4096;
                    }
                    stackSize = val;
                }
                break;
            case 'i': 
                {
                    if (!optarg) {
                        std::cerr << "No filename/region given\n";
                        continue;
                    }
                    std::string path;
                    Region region;
                    if (!parsePathWithRegion(optarg, path, region)) {
                        std::cerr << "Can't parse filename/region " << optarg << "\n";
                        continue;
                    }
                    FILE *fp = fopen(path.c_str(), "rb");
                    if (!fp) {
                        std::cerr << "Can't open file " << path << "\n";
                        continue;
                    } 
                    ImageLoader *loader = ImageLoader::get(fp);
                    fclose(fp);
                    if (loader == nullptr) {
                        std::cerr << optarg << ": Unsupported file format\n";
                        continue;
                    }
                    Image img = loader->image();
                    if (region.x != -1) {
                        img = img.extract(region.x, region.y, region.w, region.h);
                    }
                    images.push_back(img);
                    delete loader;
                }
                break;
            case 'x':
                {
                    long int val;
                    if (!parseInt(optarg, val)) {
                        std::cerr << "Invalid X position " << val << "\n";
                    } else {
                        posX = val;
                    }
                }
                break;
            case 'y':
                {
                    long int val;
                    if (!parseInt(optarg, val)) {
                        std::cerr << "Invalid Y position " << val << "\n";
                    } else {
                        posY = val;
                    }
                }
                break;
            default:
                usage();
        }
    }
    if (optind + 1 != argc) {
        // no or more than 1 filename given
        usage();
    }
    filename = argv[optind];

    // Check mandatory flags
    if (type == Type::UNDEFINED) {
        usage();
    }
    if (images.size() < 1 || images.size() > 2) {
        usage();
    }

    std::ofstream ofs;
    ofs.open(filename);
    binstream os(ofs);

    bool hasDrawerData = (type == Type::DRAWER || type == Type::DISK);
    if (type != Type::TOOL) {
        stackSize = 0;
    }

    // 0x00 UWORD ic_Magic          always 0xE310
    os << (std::uint16_t)0xe310;

    // 0x00 UWORD ic_Version        always 1
    os << (std::uint16_t)0x01;

    // 0x04 struct Gadget
    //    0x00 APTR  ga_Next           <undefined> always 0
    os << (std::uint32_t)0;

    //    0x04 WORD  ga_LeftEdge       X position of icon in drawer/on WorkBench
    os << (std::uint16_t)posX;

    //    0x06 WORD  ga_TopEdge        Y position of icon in drawer/on WorkBench
    os << (std::uint16_t)posX;

    //    0x08 WORD  ga_Width          the width of the gadget
    os << (std::int16_t)(images[0].w());

    //    0x0A WORD  ga_Height         the height of the gadget
    os << (std::int16_t)(images[0].h());

    //    0x0C UWORD ga_Flags          gadget flags
    //         bit 2                   always set (image 1 is an image ;-)
    //         bit 1                   if set, we use 2 image-mode
    //         bit 0                   if set we use backfill mode, else complement mode
    //                                 complement mode: gadget colors are inverted
    //                                 backfill mode: like complement, but region
    //                                 outside (color 0) of image is not inverted
    os << (std::uint16_t)(images.size() > 1 ? 0b110 : 0b101);

    //    0x0E UWORD ga_Activation     <undefined>
    os << (std::uint16_t)0;

    //    0x10 UWORD ga_GadgetType     <undefined>
    os << (std::uint16_t)0;

    //    0x12 APTR  ga_GadgetRender   <boolean> unused??? always true
    os << (std::uint32_t)0x1234; // byte order doesn't matter as long as it's != 0
    
    //    0x16 APTR  ga_SelectRender   <boolean> (true if second image present)
    os << (std::uint32_t)(images.size() > 1 ? 0x1234 : 0); // byte order doesn't matter as long as it's != 0

    //    0x1A APTR  ga_GadgetText     <undefined> always 0 ???
    os << (std::uint32_t)0;

    //    0x1E LONG  ga_MutualExclude  <undefined>
    os << (std::int32_t)0;

    //    0x22 APTR  ga_SpecialInfo    <undefined>
    os << (std::uint32_t)0;

    //    0x26 UWORD ga_GadgetID       <undefined>
    os << (std::uint16_t)0;

    //    0x28 APTR  ga_UserData       lower 8 bits:  0 for old, 1 for icons >= OS2.x
    // 	   		                       upper 24 bits: undefined
    os << (std::uint32_t)0;

    // 0x30 UBYTE ic_Type
    os << (std::uint8_t)type;

    // 0x31 UBYTE ic_Pad            <undefined>
    os << (std::uint8_t)0;

    // 0x32 APTR  ic_DefaultTool    <boolean>
    os << (std::uint32_t)0;

    // 0x36 APTR  ic_ToolTypes      <boolean>
    os << (std::uint32_t)0;

    // 0x3A LONG  ic_CurrentX       X position of icon in drawer/on WorkBench
    os << (std::int32_t)posX;

    // 0x3E LONG  ic_CurrentY       Y position of icon in drawer/on WorkBench
    os << (std::int32_t)posY;

    // 0x42 APTR  ic_DrawerData     <boolean>
    os << (std::uint32_t)(hasDrawerData ? 0x1234 : 0);

    // 0x46 APTR  ic_ToolWindow     <boolean>
    os << (std::uint32_t)0; // not supported yet

    // 0x4A LONG  ic_StackSize      the stack size for program execution
    //                            (values < 4096 mean 4096 is used)
    os << stackSize;

    if (hasDrawerData) {
        // 0x00 struct NewWindow        
        //     0x00 WORD  nw_LeftEdge       left edge distance of window
        os << (std::int16_t)30; // FIXME DO NOT HARDCODE

        //     0x02 WORD  nw_TopEdge        top edge distance of window
        os << (std::int16_t)30; // FIXME DO NOT HARDCODE

        //     0x04 WORD  nw_Width          the width of the window (outer width)
        os << (std::int16_t)400; // FIXME DO NOT HARDCODE

        //     0x06 WORD  nw_Height         the height of the window (outer height)
        os << (std::int16_t)200; // FIXME DO NOT HARDCODE

        //     0x08 UBYTE nw_DetailPen      always 255 ???
        os << (std::uint8_t)0xff;

        //     0x09 UBYTE nw_BlockPen       always 255 ???
        os << (std::uint8_t)0xff;

        //     0x0A ULONG nw_IDCMPFlags     <undefined>
        os << (std::uint32_t)0;

        //     0x0E ULONG nw_Flags          <undefined>
        os << (std::uint32_t)0;

        //     0x12 APTR  nw_FirstGadget    <undefined>
        os << (std::uint32_t)0;

        //     0x16 APTR  nw_CheckMark      <undefined>
        os << (std::uint32_t)0;

        //     0x1A APTR  nw_Title          <undefined>
        os << (std::uint32_t)0;

        //     0x1E APTR  nw_Screen         <undefined>
        os << (std::uint32_t)0;

        //     0x22 APTR  nw_BitMap         <undefined>
        os << (std::uint32_t)0;

        //     0x26 WORD  nw_MinWidth       <undefined> often 94, minimum window width
        os << (std::int16_t)94; // FIXME DO NOT HARDCODE

        //     0x28 WORD  nw_MinHeight      <undefined> often 65, minimum window height
        os << (std::int16_t)65; // FIXME DO NOT HARDCODE

        //     0x2A UWORD nw_MaxWidth       <undefined> often 0xFFFF, maximum window width
        os << (std::uint16_t)0xffff;

        //     0x2C UWORD nw_MaxHeight      <undefined> often 0xFFFF, maximum window width
        os << (std::uint16_t)0xffff;

        //     0x2E UWORD nw_Type           <undefined>
        os << (std::uint16_t)1;

        // 0x30 LONG  dd_CurrentX       the current X position of the drawer window
        //                              contents (this is the relative offset of the
        //                              drawer drawmap)
        os << (std::int32_t)10; // FIXME DO NOT HARDCODE

        // 0x34 LONG  dd_CurrentY       the current Y position of the drawer window contents
        os << (std::int32_t)10; // FIXME DO NOT HARDCODE
    }

    for(const Image& img : images) {
        addImage(img, os);
    }

    // DefaultTool not supported, ic_DefaultTool is always 0
    // ToolTypes not supported, ic_ToolTypes is always 0
    // ToolWindow not supported, ic_ToolWindow is always 0
    // DrawerData2 not supported

    ofs.close();
    
    return 0;
}
