#pragma once
#include <stdint.h>
#include <assert.h>
/*
PPU（图形处理器）
PPU 为 FC 生成视频输出。与 CPU 不同，PPU 芯片是为 FC 定制的，其运行频率是 CPU 的 3 倍。渲染时 PPU 在每个周期输出1个像素。

PPU 能够渲染游戏中的背景层和最多 64 个子画面（Sprite）。子画面可以由 8 x 8 或 8 x 16 像素构成。而背景则既可以延水平（X轴）方向卷动，又可以延竖直（Y轴）方向卷动。并且 PPU 还支持一种称为微调（Fine）的卷动模式，即每次只卷动 1 像素。这种卷动模式在当年可是非常了不起的技术。

背景和子画面都是由 8 x 8 像素的图形块（Tile）构成的，而图形块是定义在游戏卡 ROM 中的 Pattern Table 里的。Pattern Table 中的图形块仅指定了其所用颜色中的最后 2 比特，剩余的 2 比特来自 Attribute Table。Nametable 则指定了图形块在背景上的位置。总之，这一切看起来都要比今天的标准复杂得多，所以我不得不和合作者解释说“这不是简单的位图”。

背景的分辨率为 32 x 30 = 960 像素，由 8 x 8 像素的图形块构成。背景卷动的实现方法是再额外渲染多幅 32 x 30 像素的背景，且每幅背景都加上一个偏移量。如果同时沿 X 轴和 Y 轴卷动背景，那么最多可以有 4 幅背景处于可见状态。但是 FC 只支持 2 幅背景，因此游戏中经常使用不同的镜像模式（Mirroring Mode）来实现水平镜像或竖直镜像。

FROM::http://blog.jobbole.com/87068/
*/

//调色板数据
//alpha（参数a）通道一般用作不透明度参数。
const union sfc_palette_data {
	struct { uint8_t r, g, b, a; };
	uint32_t    data;
} sfc_stdpalette[64] = {
	{ 0x7F, 0x7F, 0x7F, 0xFF },{ 0x20, 0x00, 0xB0, 0xFF },{ 0x28, 0x00, 0xB8, 0xFF },{ 0x60, 0x10, 0xA0, 0xFF },
	{ 0x98, 0x20, 0x78, 0xFF },{ 0xB0, 0x10, 0x30, 0xFF },{ 0xA0, 0x30, 0x00, 0xFF },{ 0x78, 0x40, 0x00, 0xFF },
	{ 0x48, 0x58, 0x00, 0xFF },{ 0x38, 0x68, 0x00, 0xFF },{ 0x38, 0x6C, 0x00, 0xFF },{ 0x30, 0x60, 0x40, 0xFF },
	{ 0x30, 0x50, 0x80, 0xFF },{ 0x00, 0x00, 0x00, 0xFF },{ 0x00, 0x00, 0x00, 0xFF },{ 0x00, 0x00, 0x00, 0xFF },

	{ 0xBC, 0xBC, 0xBC, 0xFF },{ 0x40, 0x60, 0xF8, 0xFF },{ 0x40, 0x40, 0xFF, 0xFF },{ 0x90, 0x40, 0xF0, 0xFF },
	{ 0xD8, 0x40, 0xC0, 0xFF },{ 0xD8, 0x40, 0x60, 0xFF },{ 0xE0, 0x50, 0x00, 0xFF },{ 0xC0, 0x70, 0x00, 0xFF },
	{ 0x88, 0x88, 0x00, 0xFF },{ 0x50, 0xA0, 0x00, 0xFF },{ 0x48, 0xA8, 0x10, 0xFF },{ 0x48, 0xA0, 0x68, 0xFF },
	{ 0x40, 0x90, 0xC0, 0xFF },{ 0x00, 0x00, 0x00, 0xFF },{ 0x00, 0x00, 0x00, 0xFF },{ 0x00, 0x00, 0x00, 0xFF },

	{ 0xFF, 0xFF, 0xFF, 0xFF },{ 0x60, 0xA0, 0xFF, 0xFF },{ 0x50, 0x80, 0xFF, 0xFF },{ 0xA0, 0x70, 0xFF, 0xFF },
	{ 0xF0, 0x60, 0xFF, 0xFF },{ 0xFF, 0x60, 0xB0, 0xFF },{ 0xFF, 0x78, 0x30, 0xFF },{ 0xFF, 0xA0, 0x00, 0xFF },
	{ 0xE8, 0xD0, 0x20, 0xFF },{ 0x98, 0xE8, 0x00, 0xFF },{ 0x70, 0xF0, 0x40, 0xFF },{ 0x70, 0xE0, 0x90, 0xFF },
	{ 0x60, 0xD0, 0xE0, 0xFF },{ 0x60, 0x60, 0x60, 0xFF },{ 0x00, 0x00, 0x00, 0xFF },{ 0x00, 0x00, 0x00, 0xFF },

	{ 0xFF, 0xFF, 0xFF, 0xFF },{ 0x90, 0xD0, 0xFF, 0xFF },{ 0xA0, 0xB8, 0xFF, 0xFF },{ 0xC0, 0xB0, 0xFF, 0xFF },
	{ 0xE0, 0xB0, 0xFF, 0xFF },{ 0xFF, 0xB8, 0xE8, 0xFF },{ 0xFF, 0xC8, 0xB8, 0xFF },{ 0xFF, 0xD8, 0xA0, 0xFF },
	{ 0xFF, 0xF0, 0x90, 0xFF },{ 0xC8, 0xF0, 0x80, 0xFF },{ 0xA0, 0xF0, 0xA0, 0xFF },{ 0xA0, 0xFF, 0xC8, 0xFF },
	{ 0xA0, 0xFF, 0xF0, 0xFF },{ 0xA0, 0xA0, 0xA0, 0xFF },{ 0x00, 0x00, 0x00, 0xFF },{ 0x00, 0x00, 0x00, 0xFF }
};

// PPU用标志
enum sfc_ppu_flag {
	SFC_PPU2000_NMIGen = 0x80, // [0x2000]VBlank期间是否产生NMI
	SFC_PPU2000_Sp8x16 = 0x20, // [0x2000]精灵为8x16(1), 还是8x8(0)
	SFC_PPU2000_BgTabl = 0x10, // [0x2000]背景调色板表地址$1000(1), $0000(0)
	SFC_PPUFLAG_SpTabl = 0x08, // [0x2000]精灵调色板表地址$1000(1), $0000(0), 8x16模式下被忽略
	SFC_PPU2000_VINC32 = 0x04, // [0x2000]VRAM读写增加值32(1), 1(0)

	SFC_PPU2002_VBlank = 0x80, // [0x2002]垂直空白间隙标志
	SFC_PPU2002_Sp0Hit = 0x40, // [0x2002]零号精灵命中标志
	SFC_PPU2002_SpOver = 0x20, // [0x2002]精灵溢出标志
};

class sfc_ppu_t
{
public:
	sfc_ppu_t();
	~sfc_ppu_t();

	//====================================
	//内存地址库
	uint8_t* banks[0x4000 / 0x0400];
	//VRAM地址
	uint16_t vramaddr;
	//寄存器 PPUCTRL	@$2000
	uint8_t ctrl;
	//寄存器 PPUMASK	@$2001
	uint8_t mask;
	//寄存器 PPUSTATUS	@$2002
	uint8_t status;
	//寄存器 OAMADDR	@$2003
	uint8_t oamaddr;
	
	//滚动偏移
	uint8_t scroll[2];
	//滚动偏移双写位置记录
	uint8_t writex2;
	//显存读取缓冲值
	uint8_t pseudo;
	//精灵调色板索引
	uint8_t spindexes[0x20];
	//精灵数据 :2568
	uint8_t sprites[0x100];

	//====================================
	//读取PPU地址
	uint8_t sfc_read_ppu_address(uint16_t address);
	//写入PPU地址
	void sfc_write_ppu_address(uint16_t address, uint8_t data);

	//使用CPU地址空间读取PPU寄存器
	uint8_t sfc_read_ppu_register_via_cpu(uint16_t);
	//使用CPU地址空间写入PPU寄存器
	void sfc_write_ppu_register_via_cpu(uint16_t, uint8_t);

private:

};



