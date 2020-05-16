/*
README
图像平滑与边缘提取程序
1.本程序目前只支持Windows系统（使用了windows控制台程序）
2.本程序目前只支持BMP文件格式的图片（可将其他格式图片通过系统自带画图另存为BMP格式）
3.本程序尚未开发导入图片功能，需添加图片前烦请将BMP格式图片放置于相应工程文件的路径下
4.程序中使用了音乐播放功能（标准库函数），如若程序无法正常运行或运行中无音乐，烦请检查以下两点：
   (1) 工程文件目录下是否有 Sounds\Home.wav文件
   (2) 出现undefined reference to `PlaySoundA@12'错误，project -> bulid options 点linker settings
         再点add 添加winmm后ok即可。（项目-生成选项-链接器设置-链接库添加-输入winmm-ok）
5.感谢以下BGM，侵删！
   Home  —— Toby Fox 《UNDERTALE Soundtrack》
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <windows.h>
#include <time.h>
#include <mmsystem.h>//导入声音头文件
#include <io.h>     //改bug
#include <pthread.h>//改bug

#pragma comment(lib,"Winmm.lib")//播放音乐设置

#define HEIGHT 40
#define WIDTH 118

int Running = 1;//程序运行中标志
HANDLE hOutput, hOutBuf;//控制台屏幕缓冲区句柄
COORD coord = { 0,0 };
//双缓冲处理显示初始坐标
DWORD bytes = 0;

struct BMPFileName{
    char name[10][40];
    int count;
};
typedef struct BMPFileName BMPFile;
BMPFile BMP;//统计文件中BMP图片的名字和数量

struct Pointer{
    int mode;// 值为0或1,代表指针在左或右边
    int PX[2],PY[2];// 0和1模式下坐标起始点
    int PointerX,PointerY; //指针坐标
    int PXE[2];// 0和1模式下指针坐标终点
};
struct Pointer Pointer;//程序指针结构体

void InitPointer(){
    //初始化指针
    Pointer.mode   = 0;
    Pointer.PX[0]  = 17;
    Pointer.PY[0]  = 29;
    Pointer.PX[1]  = 17;
    Pointer.PY[1]  = 59;
    Pointer.PXE[0] = 19+(BMP.count)*2;
    Pointer.PXE[1] = 35;
    Pointer.PointerX = Pointer.PX[0];
    Pointer.PointerY = Pointer.PY[0];
}

/**近期热点模块**/
char news[5][80] = {
                    "美股两周内熔断4次,巴菲特连连惊呼活久见",
                    "特朗普两周内登日4次,勇夺航空第一人称号",
                    "淡黄的长裙，蓬松的头发~",
                    "韩国“N号房”事件持续发酵",
                    "瑞幸咖啡被曝伪造22亿交易额,盘前暴跌80%"
                    };
/**动态WELCOME模块**/
int WelcomeX = 0;
char Blank[200] = "                                                                                                                      ";
char Welcome[5][200] = {
        "BGM: Home                  **       *       **  ********   **         *******    *******     ****     ****   ******** ",
        "                            **     ***     **   **         **        **         *       *    ** **   ** **   **       ",
        "                             **   ** **   **    ********   **       **         *         *   **  ** **  **   ******** ",
        "                              ** **   ** **     **         **        **         *       *    **   ***   **   **       ",
        "                               ***     ***      ********   ********   *******    *******     **         **   ******** "
};
/**主界面模块**/
char data[HEIGHT][WIDTH] = {
        "",
        "",
        "",
        "",
        "",
        "                          ******************************************************************************************** ",
        "                          **                      **       **   ********   **    *   *     *                        ** ",
        "                          **                      * *     * *   **         * *   *   *     *                        ** ",
        "                          **                      *  *   *  *   ********   *  *  *   *     *                        ** ",
        "                          **                      *   * *   *   **         *   * *   *     *                        ** ",
        "                          **                      *    *    *   ********   *    **   *******                        ** ",
        "                          **----------------------------------------------------------------------------------------** ",
        "                          **                           使用 “↑ ↓”和‘Enter’来选择操作                            **",
        "                          **----------------------------------------------------------------------------------------** ",
        "                          **   工程目录中查找到   |                      选择对图片的处理方式                       ** ",
        "                          **       如下图片       |            (建议先平滑再查看边缘图像，推荐中值平滑)             ** ",
        "                          **----------------------|-----------------------------------------------------------------** ",
        "                          **                      |             查看原图                                            ** ",
        "                          **                      |                                                                 ** ",
        "                          **                      |             查看灰度图                                          ** ",
        "                          **                      |                                                                 ** ",
        "                          **                      |             查看梯度图像                                        ** ",
        "                          **                      |                                                                 ** ",
        "                          **                      |             均值滤波平滑                                        ** ",
        "                          **                      |                                                                 ** ",
        "                          **                      |             高斯滤波平滑                                        ** ",
        "                          **                      |                                                                 ** ",
        "                          **                      |             自定义卷积平滑                                      ** ",
        "                          **                      |                                                                 ** ",
        "                          **                      |             边缘保留高斯滤波平滑                                ** ",
        "                          **                      |                                                                 ** ",
        "                          **                      |             中值滤波平滑（有效去除椒盐噪声）                    ** ",
        "                          **                      |                                                                 ** ",
        "                          **                      |             查看边缘图像                                        ** ",
        "                          **                      |                                                                 ** ",
        "                          **                      |             返回图片选择                                        ** ",
        "                          **----------------------------------------------------------------------------------------** ",
        "                          **                     近期热点：                                                         ** ",
        "                          ******************************************************************************************** "
        };

/**BMP文件查找和打印至主界面模块**/
void FindBMP(){
    //在工程目录\\pic中统计BMP
    char *to_find = (char*)malloc(500*sizeof(char));
    _getcwd(to_find,500);
    strcat(to_find,"\\pic\\*.bmp");
    BMP.count = 0;
    long handle;
    struct _finddata_t fileinfo;
    handle = _findfirst(to_find,&fileinfo);
    strcpy(BMP.name[BMP.count++],fileinfo.name);
    while(!_findnext(handle,&fileinfo)){
        strcpy(BMP.name[BMP.count++],fileinfo.name);
    }
    _findclose(handle);
}

void SetBMP(){
    //打印BMP名称到主界面
    FindBMP();
    for(int i=0;i<BMP.count;i++){
        int length = strlen(BMP.name[i])-5;
        for(int j=0;j<18;j++){
            if(j<=length)
                data[i*2+17][j+32] = BMP.name[i][j];
            else
                data[i*2+17][j+32] = ' ';
        }
    }
    char s1[10] = "帮助文档";
    char s2[10] = "退出程序";
    for(int j=0;j<18;j++){
        if(j<=8){
            data[BMP.count*2+17][j+32] = s1[j];
            data[BMP.count*2+19][j+32] = s2[j];
        }
        else{
            data[BMP.count*2+17][j+32] = ' ';
            data[BMP.count*2+19][j+32] = ' ';
        }
    }
}

/**初始化界面**/
clock_t t1,t2;
void InitMenu(){
    //初始化页面设置
    if(WelcomeX>=0 && WelcomeX<=4)
        strcpy(data[WelcomeX],Welcome[WelcomeX]);
    if(WelcomeX>=10 && WelcomeX<=14){
        strcpy(data[14-WelcomeX],Blank);
    }
    WelcomeX++;
    if(WelcomeX>14)
        WelcomeX = 0;//初始化welcome

    t2 = clock();
    int index = ((t2-t1)/3000)%5;
    int newslength = strlen(news[index]);
    for(int i=0;i<40;i++){
        if(i<=newslength)
            data[37][60+i] = news[index][i];
        else
            data[37][60+i] = ' ';//初始化近期热点模块
    }
}

void SetPointer(){
    //设置指针位置
    data[Pointer.PointerX][Pointer.PointerY] = '-';
    data[Pointer.PointerX][Pointer.PointerY+1] = '-';
    data[Pointer.PointerX][Pointer.PointerY+2] = '>';
}


char help[22][66] = {
                        "                          帮助文档                           ",
                        "                    （按任意键返回菜单）                     ",
                        "-----------------------------------------------------------------",
                        "",
                        "",
                        "",
                        "      README                                                 ",
                        "      图像平滑与边缘提取程序                                 ",
                        "      1.本程序目前只支持Windows系统下使用                    ",
                        "                                                             ",
                        "      2.本程序目前只支持BMP文件格式的图片                    ",
                        "                                                             ",
                        "      3.本程序尚未开发导入图片功能，需添加图片前烦请将BMP格式",
                        "        图片放置于工程目录的pic文件夹中                      ",
                        "                                                             ",
                        "      4.感谢以下BGM，侵删！                                  ",
                        "        Home  —— Toby Fox 《UNDERTALE Soundtrack》           ",
                    };
char tempData[22][66];

void Help(){
    //把help文档的内容打印到界面
        for(int i=14;i<=35;i++){
            strncpy(data[i]+51,help[i-14],65);
        }
}

void SaveHelpData(){
    //暂时保存界面内容至tempData
    for(int i=14;i<=35;i++){
        strncpy(tempData[i-14],data[i]+51,65);
    }
}

void show(){
    //主要的界面显示函数
    InitMenu();//初始化界面
    SetPointer();//设置指针位置
    /*双缓冲区读入与输出数据*/
    for (int i=0;i<HEIGHT;i++){
        coord.Y = i;
        WriteConsoleOutputCharacterA(hOutBuf, data[i], WIDTH, coord, &bytes);
    }
    //设置新的缓冲区为活动显示缓冲
    SetConsoleActiveScreenBuffer(hOutBuf);
    Sleep(60);

    for (int i=0;i<HEIGHT;i++){
        coord.Y = i;
        WriteConsoleOutputCharacterA(hOutput, data[i], WIDTH, coord, &bytes);
    }
    //设置新的缓冲区为活动显示缓冲
    SetConsoleActiveScreenBuffer(hOutput);
    Sleep(60);
}

/**图像处理模块**/
int W,H;//图像的宽和高
int HighLimit=100,LowLimit=50;//默认值： RMS和50
char **img;//图像数据储存数组
char **tempImg;//临时图像数据储存数组
int flag = 0;//是否需要显示图像
char resFilename[10] = "res0.bmp";//生成处理后图像的名称

/**核心函数列表**/
char** ReadBMP(char* filename);
    //读取BMP图像，返回存有图像数据的二维char数组
void WriteBMP(char**img,const char* filename);
    //生成BMP图片，本程序对24位真彩图简单处理，不调用结构体存入数据。
char** Copy(char**src);
    //返回一个和src数据一样的二维数组
int change(char n);
    //对图像小于0的数据进行处理
char** GrayImg(char**img);
    //获取img的灰度图，返回单通道灰度图像数据
int** TiImg(char** Gray);
    //利用Sobel算子计算出灰度图的梯度图
void GrayLv(char**img);
    //将img转换为灰度图
void TiLv(char** img);
    //将img转换成梯度图像
void AverageLv(char**img);
    //均值滤波
void GaosiLv(char **img);
    //高斯过滤
void idyLv(char**img, int Juanji[3][3]);
    //自定义卷积滤波 3*3卷积模板
void EdgeGaosiLv(char**img);
    //高斯滤波 5*5卷积模板
void MidValueLv(char**img);
    //中值过滤
int Round(int**Ti,int i,int j);
    //寻找梯度图像中八个方位是否有梯度大于高阈值的点
void Canny(char**img,int N);
    //高于高阈值的点视为边缘，高于低阈值低于高阈值的点，若Round返回值为1则视为边缘点
int RMS(char**img);
    //返回img梯度图像数据的均方根
void DFS(int i,int j, int EI);
    //深度搜索算法
void DFSImg(char**img);
    //深度搜索图像的边缘

/**具体实现**/
char** ReadBMP(char* filename){
    //读取BMP图像，返回存有图像数据的二维char数组

    FILE *fp=fopen(filename,"rb");//二进制读方式打开指定的图像文件
    int a[200];//存BMP图像的部分数据
    char t[2]; //存BMP图像字符串头"BM"
    fread(t,2,1,fp);
    fread(a,52,1,fp);
    W = a[4]; // 宽
    H = a[5]; // 高
    //获取图像宽、高信息

    //下面完成图像数据向内存数组的存储
    char** ImageData = (char**)malloc(H*sizeof(char*));
    for (int i=0;i<H;i++){
        ImageData[i]=(char*)malloc((W*3+3)/4*4*sizeof(char));
    }
    for (int k=0;k<H;k++ ){
        for(int j=0;j<(W*3+3)/4*4;j++){
            fread(&ImageData[k][j],1,1,fp);//上面完成动态二维数组的申请，这里实际读取图像数据
        }
    }
    fclose(fp);//关闭文件
    return ImageData;
}

void WriteBMP(char**img,const char* filename)
{   //生成BMP图片，本程序对24位真彩图简单处理，不调用结构体存入数据。
    int l=(W*3+3)/4*4;
    int bmi[]= {l*H+54,0,54,40,W,H,1|3*8<<16,0,l*H,0,0,100,0};//根据BMP格式信息填入
    FILE *fp = fopen(filename,"wb");
    fprintf(fp,"BM");
    fwrite(&bmi,52,1,fp);
    for(int i=0;i<H;i++){
        fwrite(img[i],1,l,fp);
    }
    fclose(fp);
}

char** Copy(char**src){
    //返回一个和src数据一样的二维数组
    int l = (W*3+3)/4*4;
    char**tar;
    tar = (char**)malloc(H*sizeof(char*));
    for (int i=0;i<H;i++){
        tar[i]=(char*)malloc(l*sizeof(char));
    }
    for(int i=0;i<H;i++)
        for(int j=0;j<l;j++)
            tar[i][j] = src[i][j];
    return tar;
}

int change(char n){
    //对图像小于0的数据进行处理
    if(n<0) return 256+n;
    else return n;
}

char** GrayImg(char**img){
    //获取img的灰度图，返回单通道灰度图像数据
    char** Gray = (char**)malloc(H*sizeof(char*));
    for(int i=0;i<H;i++){
        Gray[i] = (char*)malloc(W*sizeof(char));
    }
    /* G = 0.3*B+0.59*G+0.11*R */
    for(int i=0;i<H;i++){
        for(int j=0;j<W;j++){
            Gray[i][j] = (change(img[i][j*3])*30 + change(img[i][j*3+1])*59 + change(img[i][j*3+2])*11)/100;
            if(Gray[i][j]>=128){
                Gray[i][j] -= 256;
            }
        }
    }
    return Gray;
}

int** TiImg(char** Gray){
    //利用Sobel算子计算出灰度图的梯度图
    int** Ti = (int**)malloc(H*sizeof(int*));
    for(int i=0;i<H;i++){
        Ti[i] = (int*)malloc(W*sizeof(int));
    }
    for(int j=0;j<W;j++){
        Ti[0][j] = 0;
        Ti[H-1][j] = 0;
    }
    for(int i=0;i<H;i++){
        Ti[i][0] = 0;
        Ti[i][W-1] = 0;
    }
    int JuanjiX[3][3] = { {-1,0,1},{-2,0,2},{-1,0,1} };
    int JuanjiY[3][3] = { {-1,-2,-1},{0,0,0},{1,2,1} };//Sobel横纵向算子
    for(int i=1;i<H-1;i++){
        for(int j=1;j<W-1;j++){
            int Gx = 0;
            for(int x=i-1;x<=i+1;x++)
                for(int y=j-1;y<=j+1;y++)
                    Gx += change(Gray[x][y])*JuanjiX[x-i+1][y-j+1];
            int Gy = 0;
            for(int x=i-1;x<=i+1;x++)
                for(int y=j-1;y<=j+1;y++)
                    Gy += change(Gray[x][y])*JuanjiY[x-i+1][y-j+1];

            Ti[i][j] = (int)sqrt(Gx*Gx+Gy*Gy);
            if(Ti[i][j]>255) Ti[i][j] = 255;
        }
    }
    return Ti;
}

void GrayLv(char**img){
    //将img转换为灰度图
    char** Gray = GrayImg(img);
    for(int i=1;i<H-1;i++){
        for(int j=1;j<W-1;j++){
            img[i][j*3] = img[i][j*3+1] = img[i][j*3+2] = Gray[i][j];
        }
    }
}

void TiLv(char** img){
    //将img转换成梯度图像
    char**Gray = GrayImg(img);
    int** Ti = TiImg(Gray);
    for(int i=1;i<H-1;i++){
        for(int j=1;j<W-1;j++){
            img[i][j*3] = img[i][j*3+1] = img[i][j*3+2] = Ti[i][j];
        }
    }
}

void AverageLv(char**img){
    //均值滤波
    int** Ti = TiImg(GrayImg(img));
    for(int i=2;i<H-2;i++){
        for(int j=2;j<W-2;j++){
            for(int k=0;k<3;k++){
                int sum = 0;
                for(int x=i-2;x<=i+2;x++)
                    for(int y=j-2;y<=j+2;y++){
                        int value = img[x][y*3+k];
                        if(value<0){
                            value += 256;
                        }
                        sum += value;
                    }
                int res = sum/25;
                if(res>=255){
                    res = 255;
                }
                img[i][j*3+k] = res;
            }
        }
    }
}

void GaosiLv(char **img){
    //高斯过滤
    int** Ti = TiImg(GrayImg(img));
    int Juanji[5][5] = {
                        {1,4,7,4,1},
                        {4,16,26,16,4},
                        {7,26,41,26,7},
                        {4,16,26,16,4},
                        {1,4,7,4,1}
                    };//高斯卷积核
    for(int i=2;i<H-2;i++){
        for(int j=2;j<W-2;j++){
            for(int k=0;k<3;k++){
                int sum = 0;
                for(int x=i-2;x<=i+2;x++)
                    for(int y=j-2;y<=j+2;y++){
                        int value = img[x][y*3+k];
                        if(value < 0){
                            value += 256;
                        }
                        sum += value*Juanji[x-i+2][y-j+2];
                    }
                int res = sum/273;
                if(res>=128){
                    res -= 256;
                }
                img[i][j*3+k] = res;
            }
        }
    }
}

void idyLv(char**img, int Juanji[3][3]){
    //自定义卷积滤波 3*3卷积模板
    int l = (W*3+3)/4*4;
    int** Ti = TiImg(GrayImg(img));
    int JuanjiSum = 0;
    for(int i=0;i<3;i++)
        for(int j=0;j<3;j++)
            JuanjiSum += Juanji[i][j];
    for(int i=1;i<H-1;i++){
        for(int j=1;j<W-1;j++){
            if(Ti[i][j]<HighLimit)
                for(int k=0;k<3;k++){
                    int sum = 0;
                    for(int x=i-1;x<=i+1;x++)
                        for(int y=j-1;y<=j+1;y++){
                            int value = img[x][y*3+k];
                            if(value < 0){
                                value += 256;
                            }
                            sum += value*Juanji[x-i+1][y-j+1];
                        }
                    int res = sum/JuanjiSum;
                    if(res>=128){
                        res -= 256;
                    }
                    img[i][j*3+k] = res;
                }
        }
    }
}

void EdgeGaosiLv(char**img){
    //高斯滤波 5*5卷积模板
    int** Ti = TiImg(GrayImg(img));
    int Juanji[5][5] = {
                        {1,4,7,4,1},
                        {4,16,26,16,4},
                        {7,26,41,26,7},
                        {4,16,26,16,4},
                        {1,4,7,4,1}
                    };
    for(int i=2;i<H-2;i++){
        for(int j=2;j<W-2;j++){
            if(Ti[i][j]<HighLimit)//如果梯度值大于高阈值，不进行高斯模糊
                for(int k=0;k<3;k++){
                    int sum = 0;
                    for(int x=i-2;x<=i+2;x++)
                        for(int y=j-2;y<=j+2;y++){
                            int value = img[x][y*3+k];
                            if(value < 0){
                                value += 256;
                            }
                            sum += value*Juanji[x-i+2][y-j+2];
                        }
                    int res = sum/273;
                    if(res>=128){
                        res -= 256;
                    }
                    img[i][j*3+k] = res;
                }
        }
    }
}

int cmp(const void*a, const void*b){
    //快速排序比较函数
    return *(int*)a - *(int*)b;
}

void MidValueLv(char**img){
    //中值过滤
    int l = (W*3+3)/4*4;
    for(int i=1;i<H-1;i++){
        for(int j=3;j+3<l;j++){
            int count = 0;
            int value[9] = {0};
            for(int x=i-1;x<=i+1;x++)
                for(int y=j-3;y<=j+3;y+=3)
                    value[count++] = change(img[x][y]);
            qsort(value,9,sizeof(int),cmp);
            if(value[4]>=128){
                value[4] -= 256;
            }
            img[i][j] = value[4];
        }
    }
}

int Round(int**Ti,int i,int j){
    //寻找梯度图像中八个方位是否有梯度大于高阈值的点
    for(int x=i-1;x<=i+1;x++)
        for(int y=j-1;y<=j+1;y++)
            if(Ti[x][y] >= HighLimit)
                return 1;
    return 0;
}

void Canny(char**img, int N){
    //高于高阈值的点视为边缘点，高于低阈值低于高阈值的点，若Round返回值为1则视为边缘点
    //根据N(1-9)的值改变HighLimit的值,从而改变边缘数;1-5-9:少-正常-多
    int l = (W*3+3)/4*4;
    char** Gray = GrayImg(img);
    HighLimit += (5-N)*10;
    int** Ti = TiImg(Gray);
    for(int i=1;i<H-1;i++){
        for(int j=1;j<W-1;j++){
            if(Ti[i][j]>=HighLimit)
                img[i][j*3] = img[i][j*3+1] = img[i][j*3+2] = Ti[i][j];
            else if(Ti[i][j]>=LowLimit && Round(Ti,i,j))
                img[i][j*3] = img[i][j*3+1] = img[i][j*3+2] = Ti[i][j];
            else
                img[i][j*3] = img[i][j*3+1] = img[i][j*3+2] = 0;
        }
    }
}

int RMS(char**img){
    //求图像数据的均方根RMS
    int **Ti = TiImg(GrayImg(img));
    long long sum = 0;
    for(int i=1;i<H-1;i++)
        for(int j=1;j<W-1;j++)
            sum += Ti[i][j]*Ti[i][j];
    long long RMS = sqrt(sum/((W-2)*(H-2)));
    return RMS;
}

int** visited;      //梯度图像的地图化
int EdgeLength[5000];   //边缘长度
int EdgeIndex = 0;      //边缘数组下标
void DFS(int i,int j,int EI){
    //深度搜索算法
    if(visited[i][j] == 1) //已访问过
            return;
    visited[i][j] = 1;
    EdgeLength[EI]++;
    //八连通查找
    if(i-1>=1){
        if(j-1>=1 && visited[i-1][j-1]==0)
            DFS(i-1,j-1,EI);//左上

        if(j+1<=W-1 && visited[i-1][j+1]==0)
            DFS(i-1,j+1,EI);//右上

        if(visited[i-1][j]==0)
            DFS(i-1,j,EI);//正上
    }
    if(i+1<=H-1){
        if(j-1>=1 && visited[i+1][j-1]==0)
            DFS(i+1,j-1,EI);//左下

        if(j+1<=W-1 && visited[i+1][j+1]==0)
            DFS(i+1,j+1,EI);//右下

        if(visited[i+1][j]==0)
            DFS(i+1,j,EI);//正下
    }
    if(j-1>=1 && visited[i][j-1]==0)
            DFS(i,j-1,EI);//左

    if(j+1<=W-1 && visited[i][j+1]==0)
            DFS(i,j+1,EI);//右
}

void DFSImg(char**img){
    //深度搜索图像的边缘
    memset(EdgeLength,0,sizeof(EdgeLength));
    visited = (int**)malloc(H*sizeof(int*));
    for(int i=0;i<H;i++)
        visited[i] = (int*)malloc(W*sizeof(int));//为visited数组申请内存空间
    int** Ti = TiImg(GrayImg(img));
    for(int i=1;i<H-1;i++){
        for(int j=1;j<W-1;j++)
            if(Ti[i][j]>=HighLimit || (Ti[i][j]>=LowLimit && Round(Ti,i,j)))
                visited[i][j] = 0;//可访问
            else
                visited[i][j] = -1;//不可访问
    }
    for(int i=1;i<H-1;i++)
        for(int j=1;j<W-1;j++)
            if(visited[i][j]==0){
                DFS(i,j,EdgeIndex);
                EdgeIndex++;
            }
}

//键盘输入操作
int JuanjiFlag = 0;//是否自定义卷积核
int JuanCount = 0; //卷积核比例和
int EdgeFlag = 0;  //是否查看边缘
int idyJuan[3][3]; //自定义卷积核
int HelpFlag = 0;  //是否打开帮助文档
void Moving(){
    if(_kbhit()){
        //若键盘有输入

        if(HelpFlag == 1){
            //打开帮助文档
            _getch();
            for(int i=14;i<=35;i++)
                strncpy(data[i]+51,tempData[i-14],65);
            HelpFlag = 0;
        }

  else {
            //打开帮助文档外的选择
        char temp = _getch();
        switch(temp){
            case 72 :   data[Pointer.PointerX][Pointer.PointerY] = data[Pointer.PointerX][Pointer.PointerY+1] = data[Pointer.PointerX][Pointer.PointerY+2] = ' ';
                        if(Pointer.PointerX-2 >= Pointer.PX[Pointer.mode])
                            Pointer.PointerX -= 2;
                        else
                            Pointer.PointerX = Pointer.PXE[Pointer.mode];
                        break;
                        //键盘输入↑

            case 80 :   data[Pointer.PointerX][Pointer.PointerY] = data[Pointer.PointerX][Pointer.PointerY+1] = data[Pointer.PointerX][Pointer.PointerY+2] = ' ';
                        if(Pointer.PointerX+2 <= Pointer.PXE[Pointer.mode])
                            Pointer.PointerX += 2;
                        else
                            Pointer.PointerX = Pointer.PX[Pointer.mode];
                        break;
                        //键盘输入↓

            case 13:    data[Pointer.PointerX][Pointer.PointerY] = data[Pointer.PointerX][Pointer.PointerY+1] = data[Pointer.PointerX][Pointer.PointerY+2] = ' ';
                        if(Pointer.mode==0) {
                            //指针在左边时
                            if(Pointer.PointerX == Pointer.PXE[0]){
                                Running = 0;
                            }
                            else if(Pointer.PointerX == Pointer.PXE[0]-2){
                                SaveHelpData();
                                Help();
                                HelpFlag = 1;
                            }
                            else{
                                char srcFilename[30] = "pic\\";
                                strcat(srcFilename,BMP.name[(Pointer.PointerX-Pointer.PX[0])/2]);
                                img = ReadBMP(srcFilename);
                                tempImg = Copy(img);
                                Pointer.mode = 1;
                                Pointer.PointerX = Pointer.PX[Pointer.mode];
                                Pointer.PointerY = Pointer.PY[Pointer.mode];
                            }
                        }
                        else {
                            //键盘在右边时
                            if(Pointer.PointerX == Pointer.PXE[1]){
                                Pointer.mode = 0;
                                Pointer.PointerX = Pointer.PX[Pointer.mode];
                                Pointer.PointerY = Pointer.PY[Pointer.mode];
                            }
                            else {
                                //生成文件名命名
                                resFilename[3]++;
                                if(resFilename[3]-'0'>=10)
                                    resFilename[3] = '0';

                                switch( (Pointer.PointerX-Pointer.PX[1])/2 ){
                                    case 0: WriteBMP(img,resFilename);
                                            flag = 1;
                                            break;
                                    case 1: tempImg = Copy(img);
                                            GrayLv(tempImg);
                                            WriteBMP(tempImg,resFilename);
                                            flag = 1;
                                            break;
                                    case 2: tempImg = Copy(img);
                                            TiLv(tempImg);
                                            WriteBMP(tempImg,resFilename);
                                            flag = 1;
                                            break;
                                    case 3: tempImg = Copy(img);
                                            AverageLv(tempImg);
                                            WriteBMP(tempImg,resFilename);
                                            flag = 1;
                                            break;
                                    case 4: tempImg = Copy(img);
                                            GaosiLv(tempImg);
                                            WriteBMP(tempImg,resFilename);
                                            flag = 1;
                                            break;
                                    case 5: tempImg = Copy(img);
                                            JuanjiFlag = 1;
                                            char Juanji[8][30] = {
                                                "请输入3*3(0-9)整数矩阵",
                                                "----------------------",
                                                "|      |      |      |",
                                                "----------------------",
                                                "|      |      |      |",
                                                "----------------------",
                                                "|      |      |      |",
                                                "----------------------",
                                            };
                                            for(int i=0;i<8;i++)
                                                for(int j=0;j<22;j++)
                                                    data[i+17][88+j] = Juanji[i][j];
                                            break;
                                    case 6: tempImg = Copy(img);
                                            EdgeGaosiLv(tempImg);
                                            WriteBMP(tempImg,resFilename);
                                            flag = 1;
                                            break;
                                    case 7: tempImg = Copy(img);
                                            MidValueLv(tempImg);
                                            WriteBMP(tempImg,resFilename);
                                            flag = 1;
                                            break;
                                    case 8: EdgeFlag = 1;
                                            char Edge[8][28] = {
                                                " 边缘数级别0-5-9:少-正常-多",
                                                "----------------------------",
                                                "|请输入边缘数级别N:        |",
                                                "----------------------------",
                                                "|图像总边缘数量为:         |",
                                                "|最长的边缘长度为:         |",
                                                "|最短的边缘长度为:         |",
                                                "----------------------------",
                                            };
                                            for(int i=0;i<8;i++)
                                                for(int j=0;j<28;j++)
                                                    data[i+17][82+j] = Edge[i][j];
                                            break;
                                    default: break;
                                }
                            }
                        }
                        break;
                        //键盘输入‘Enter’
        }

        //自定义卷积操作
        if(JuanjiFlag == 2 || (JuanjiFlag==1 && (temp==72||temp==80))){
            //按下任意键取消自定义卷积信息显示
            for(int i=0;i<8;i++)
                for(int j=0;j<22;j++)
                    data[i+17][88+j] = ' ';
            JuanjiFlag = 0;
            JuanCount = 0;
        }
        if( JuanjiFlag==1 && temp>='0'&&temp<='9'){
            int Juan[9];
            data[(JuanCount/3)*2+19][(JuanCount)%3*7+91] = temp;
            idyJuan[JuanCount/3][JuanCount%3] = temp-'0';
            JuanCount++;
            if(JuanCount==9) {
                JuanCount = 0;
                JuanjiFlag = 2;
                idyLv(tempImg,idyJuan);
                WriteBMP(tempImg,resFilename);
                flag = 1;
            }
        }

        //查看边缘操作
        if(EdgeFlag == 2 || (EdgeFlag==1 && (temp==72||temp==80))){
            //按下任意键取消边缘信息显示
            for(int i=0;i<8;i++)
                for(int j=0;j<28;j++)
                    data[i+17][82+j] = ' ';
            EdgeFlag = 0;
        }
        if( EdgeFlag==1 && temp>='0'&&temp<='9'){
            int N = 5;//默认边缘数级别为5

            data[19][106] = temp; //将N显示到界面
            N = temp-'0';
            DFSImg(tempImg);        //对图像深度搜索
            int num = EdgeIndex;
            qsort(EdgeLength,EdgeIndex,sizeof(int),cmp);//对边缘长度进行排序
            int MinL = EdgeLength[0];
            int MaxL = EdgeLength[EdgeIndex-1];
            for(int i=0;i<6;i++){
                if(num){
                    data[21][106-i] = (num%10)+'0';
                    num /= 10;
                }
                if(MaxL){
                    data[22][106-i] = (MaxL%10)+'0';
                    MaxL /= 10;
                }
                if(MinL){
                    data[23][106-i] = (MinL%10)+'0';
                    MinL /= 10;
                }
            }
            EdgeIndex = 0;
            Canny(tempImg,N);
            WriteBMP(tempImg,resFilename);
            flag = 1;
            EdgeFlag = 2;
        }
    }
    }
}

int Emoji = 1;
int direction = 1;
void BYE(){
    //退出程序
    int height = 8;
    char bye[8][200] = {
        "  *****    *    *   ******      *****    *    *   ******     ",
        "  **   *    *  *    **          **   *    *  *    **         ",
        "  *****      **     ******      *****      **     ******     ",
        "  **   *     **     **          **   *     **     **         ",
        "  *****      **     ******      *****      **     ******     ",
        "                        按任意键退出                         ",
        "                                                             "
    };
    /*对表情头进行操作*/
    Emoji += direction;
    strncpy(bye[height-1]+Emoji,"\\(^.^)/",7);
    if(Emoji == 50){
        direction = -1;
    }
    if(Emoji == 1){
        direction = 1;
    }

    /*双缓冲区读入与输出数据*/
    COORD coord = {40,0};
    for (int i=0;i<height;i++){
        coord.Y = i+15;
        WriteConsoleOutputCharacterA(hOutBuf, bye[i], WIDTH, coord, &bytes);
    }
    //设置新的缓冲区为活动显示缓冲
    SetConsoleActiveScreenBuffer(hOutBuf);
    Sleep(60);

    if(Emoji%5==0){
        //设置字体闪烁
        strncpy(bye[5]+23,"            ",13);
    }
    for (int i=0;i<height;i++){
        coord.Y = i+15;
        WriteConsoleOutputCharacterA(hOutput, bye[i], WIDTH, coord, &bytes);
    }
    //设置新的缓冲区为活动显示缓冲
    SetConsoleActiveScreenBuffer(hOutput);
    Sleep(60);
}

void OpenHandle(){
    //获取控制台屏幕缓冲区句柄
    hOutBuf = CreateConsoleScreenBuffer(
        GENERIC_WRITE,//定义进程可以往缓冲区写数据
        FILE_SHARE_WRITE,//定义缓冲区可共享写权限
        NULL,
        CONSOLE_TEXTMODE_BUFFER,
        NULL
    );
    hOutput = CreateConsoleScreenBuffer(
        GENERIC_WRITE,//定义进程可以往缓冲区写数据
        FILE_SHARE_WRITE,//定义缓冲区可共享写权限
        NULL,
        CONSOLE_TEXTMODE_BUFFER,
        NULL
    );
}

void HideCursor(){
    //隐藏控制台光标
    CONSOLE_CURSOR_INFO cci;
    cci.bVisible = 0;
    cci.dwSize = 1;
    SetConsoleCursorInfo(hOutput, &cci);
    SetConsoleCursorInfo(hOutBuf, &cci);
}

void InitAll(){
    //PlaySound("Sounds\\Home.wav",NULL,SND_FILENAME | SND_ASYNC | SND_LOOP);
    //播放音乐，若出错，请阅读代码开头 或 README.txt 或注释掉该行代码
    t1 = clock();
    //记录程序开始时间
    system("mode con cols=140 lines=42");
    //设置控制台窗口大小
    SetConsoleTitle("图像处理");
    //设置控制台标题
    system("color 0A");
    //设置控制台字体和背景颜色
    OpenHandle();
    //创建新的控制台缓冲区
    HideCursor();
    //隐藏两个缓冲区的光标
    SetBMP();
    //查找并输出可操作BMP文件名
    InitPointer();
    //初始化指针
}

int main(){
    InitAll();
    //初始化程序设置（BGM,控制台,BMP查找等）

    while(Running){
        //程序运行中
        show();
        Moving();
        if(flag){
            //当选择显示图片时
            CloseHandle(hOutBuf);
            CloseHandle(hOutput);
            //关闭两个缓冲屏句柄
            system(resFilename);
            OpenHandle();
            //重新打开句柄
            HideCursor();
            flag = 0;
        }
    }

    //退出程序
    CloseHandle(hOutBuf);
    CloseHandle(hOutput);
    OpenHandle();
    HideCursor();
    while(!_kbhit()){
        //按任意键退出程序
        BYE();
    }
    return 0;
}
