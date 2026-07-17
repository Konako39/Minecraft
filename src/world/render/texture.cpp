// 整个工程只允许这一处写 STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>



//stb_image 是「单头文件库」：函数声明和实现都在一个 .h 里。
//#define STB_IMAGE_IMPLEMENTATION 是开关——定义了它再 include，实现代码才会展开。
//而且全工程只能开一次 这里就是他的家了