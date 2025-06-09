#include <graphics.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <windows.h>
#include <mmsystem.h> // 添加多媒体库以支持音频播放
#include <ctime> // 添加时间头文件以使用 time 函数
#pragma comment(lib, "winmm.lib")  // 链接 Windows 多媒体库

// 定义障碍物结构体
struct Obstacle {
    int x, y;
    int width, height;
    COLORREF color;
};

// 定义死亡区域结构体
struct DeathZone {
    int x, y;
    int width, height;
};

// 定义透明区域结构体
struct TransparentZone {
    int x, y;
    int width, height;
};

// 定义蘑菇怪结构体
struct Mogu {
    int x, y;
    int width, height;
    int moveSpeed;
    bool facingRight;
    int minX, maxX; // 移动范围
    IMAGE imgLeft, imgLeftMask;
    IMAGE imgRight, imgRightMask;
};

// 定义乌鸦结构体
struct Wuya {
    int x, y;
    int width, height;
    int moveSpeed;
    bool facingRight;
    int minX, maxX; // 移动范围
    bool isNeutral;   // 是否为中立状态（白天）
    IMAGE imgDaytime, imgDaytimeMask;     // 白天图片
    IMAGE imgNightLeft, imgNightLeftMask; // 夜晚左向图片
    IMAGE imgNightRight, imgNightRightMask; // 夜晚右向图片
};

void PlayCharacterVoice() {
    // 停止之前的语音（如果有）
    mciSendString("close voice", nullptr, 0, nullptr);
    // 打开并播放新的语音
    char cmd[512];
    sprintf(cmd, "open \"D:\\Cyber-Space\\c++\\Gemshin-games\\video\\aqiao.wav\" type mpegvideo alias voice");
    mciSendString(cmd, nullptr, 0, nullptr);
    mciSendString("play voice", nullptr, 0, nullptr);
}

void PlayBackgroundMusic() {
    // 停止之前的背景音乐（如果有）
    mciSendString("close bgm", nullptr, 0, nullptr);
    
    // 背景音乐文件路径数组
    const char* bgmFiles[] = {
        "D:\\Cyber-Space\\c++\\Gemshin-games\\video\\yanmi_zhu_yewan1.wav",
        "D:\\Cyber-Space\\c++\\Gemshin-games\\video\\huayu_hui_baitian1.wav",
        "D:\\Cyber-Space\\c++\\Gemshin-games\\video\\yanmi_zhu_baitian1.wav"
    };
    
    // 随机选择背景音乐
    int randomIndex = rand() % 3;
    
    // 打开并循环播放背景音乐
    char cmd[512];
    sprintf(cmd, "open \"%s\" type mpegvideo alias bgm", bgmFiles[randomIndex]);
    mciSendString(cmd, nullptr, 0, nullptr);
    mciSendString("play bgm repeat", nullptr, 0, nullptr);
}

// 添加绘制重新开始按钮的函数
void DrawRestartButton(int x, int y, int size, bool isHovered) {
    // 设置颜色（悬停时更亮）
    setlinecolor(isHovered ? RGB(255, 255, 255) : RGB(200, 200, 200));
    setfillcolor(isHovered ? RGB(100, 100, 100) : RGB(80, 80, 80));

    // 绘制圆形背景
    fillcircle(x, y, size);
    
    // 绘制箭头
    POINT arrow[3];
    // 箭头尖端
    arrow[0].x = x + size/2;
    arrow[0].y = y;
    // 箭头底部两点
    arrow[1].x = x - size/4;
    arrow[1].y = y + size/3;
    arrow[2].x = x - size/4;
    arrow[2].y = y - size/3;
    
    setfillcolor(isHovered ? RGB(255, 255, 255) : RGB(200, 200, 200));
    solidpolygon(arrow, 3);
    
    // 绘制弧形
    int arcSize = size * 1.2;
    arc(x - arcSize/2, y - arcSize/2, x + arcSize/2, y + arcSize/2, 0, 3.14159/2);
}

// 获取最近的重生点
void GetNearestRespawnPoint(int currentX, int currentY, bool facingRight, int& respawnX, int& respawnY, int currentLevel) {
    // 定义重生点列表
    struct RespawnPoint {
        int x, y;
    };

    if (currentLevel == 1) {
        // 第一关重生点
        RespawnPoint respawnPoints[] = {
            {50, 340},      // 起始点
            {220, 340},     // 第一个坑前
            {420, 340},     // 第二个坑前
            {680, 340}      // 最后一段
        };

        // 根据朝向选择重生点
        if (facingRight) {
            // 如果角色朝右，找左边最近的重生点
            int selectedX = 50;  // 默认使用最左边的重生点
            int selectedY = 340;

            for (const auto& point : respawnPoints) {
                if (point.x < currentX && point.x > selectedX) {
                    selectedX = point.x;
                    selectedY = point.y;
                }
            }
            respawnX = selectedX;
            respawnY = selectedY;
        } else {
            // 如果角色朝左，找右边最近的重生点
            int selectedX = 680;  // 默认使用最右边的重生点
            int selectedY = 340;

            for (const auto& point : respawnPoints) {
                if (point.x > currentX && point.x < selectedX) {
                    selectedX = point.x;
                    selectedY = point.y;
                }
            }
            respawnX = selectedX;
            respawnY = selectedY;
        }
    } else if (currentLevel == 2) {
        // 第二关重生点（避开障碍物）
        RespawnPoint respawnPoints[] = {
            {53, 390},      // 起始点
            {200, 390},     // 小凸起前
            {450, 390},     // 竖台阶前
            {600, 390},     // 乌鸦区域前
            {850, 390}      // 右侧区域
        };

        // 根据朝向选择重生点
        if (facingRight) {
            // 如果角色朝右，找左边最近的重生点
            int selectedX = 53;  // 默认使用最左边的重生点
            int selectedY = 390;

            for (const auto& point : respawnPoints) {
                if (point.x < currentX && point.x > selectedX) {
                    selectedX = point.x;
                    selectedY = point.y;
                }
            }
            respawnX = selectedX;
            respawnY = selectedY;
        } else {
            // 如果角色朝左，找右边最近的重生点
            int selectedX = 850;  // 默认使用最右边的重生点
            int selectedY = 390;

            for (const auto& point : respawnPoints) {
                if (point.x > currentX && point.x < selectedX) {
                    selectedX = point.x;
                    selectedY = point.y;
                }
            }
            respawnX = selectedX;
            respawnY = selectedY;
        }
    }
}

int main() {
    // 设置控制台窗口字符编码为UTF-8
    SetConsoleOutputCP(CP_UTF8);
    
    // 初始化随机种子
    srand(static_cast<unsigned int>(time(NULL)));
    
    // 初始化窗口，大小为 1067x600
    int width = 1067;
    int height = 600;
    initgraph(width, height);

    // 添加生命系统
    int lives = 3;
    bool isInvincible = false;  // 死亡后的短暂无敌时间
    int invincibleTimer = 0;    // 无敌时间计时器
    const int INVINCIBLE_DURATION = 100;  // 无敌持续时间（约2秒）
    IMAGE heartFull, heartEmpty, heartMask;
    loadimage(&heartFull, "D:\\Cyber-Space\\c++\\Gemshin-games\\pictures\\heart_full.png", 30, 30);
    loadimage(&heartEmpty, "D:\\Cyber-Space\\c++\\Gemshin-games\\pictures\\heart_empty.png", 30, 30);
    loadimage(&heartMask, "D:\\Cyber-Space\\c++\\Gemshin-games\\pictures\\heart-mask.png", 30, 30);

    // 添加调试模式开关
    bool showObstacles = false;
    bool F1Pressed = false;

    // 加载首页图片
    IMAGE homepageImg;
    loadimage(&homepageImg, "D:\\Cyber-Space\\c++\\Gemshin-games\\pictures\\homepage.png", width, height);
    
    // 绘制开始按钮
    int buttonWidth = 100;
    int buttonHeight = 100;
    int buttonX = width/2 - buttonWidth/2;
    int buttonY = height/2 - buttonHeight/2;
    
    bool hover = false;
    bool gameStarted = false;
    
    BeginBatchDraw(); // 开始批量绘图
    
    while(!gameStarted) {
        // 清空屏幕
        putimage(0, 0, &homepageImg);
        
        // 获取鼠标位置
        ExMessage msg;
        if(peekmessage(&msg, EM_MOUSE)) {
            // 检测鼠标是否在按钮上
            if(msg.x > buttonX && msg.x < buttonX + buttonWidth &&
               msg.y > buttonY && msg.y < buttonY + buttonHeight) {
                hover = true;
                if(msg.message == WM_LBUTTONDOWN) {
                    // 优化点击动画效果
                    for(int i = 0; i < 30; i++) {
                        putimage(0, 0, &homepageImg);
                        setfillcolor(RGB(255-i*8, 255-i*8, 255-i*8));
                        solidcircle(width/2, height/2, (30-i)*8);
                        FlushBatchDraw(); // 刷新绘图
                        Sleep(5);
                    }
                    gameStarted = true;
                    break;
                }
            } else {
                hover = false;
            }
        }
        
        // 绘制圆形开始按钮
        if(hover) {
            setfillcolor(RGB(80, 180, 255)); // 悬停时颜色稍深
        } else {
            setfillcolor(RGB(100, 200, 255)); // 正常颜色
        }
        solidcircle(width/2, height/2, buttonWidth/2);
        
        // 绘制三角形播放图标
        setfillcolor(WHITE);
        POINT pts[] = {
            {width/2 - 20, height/2 - 25},
            {width/2 - 20, height/2 + 25},
            {width/2 + 30, height/2}
        };
        solidpolygon(pts, 3);
        
        FlushBatchDraw(); // 刷新绘图
        Sleep(10);
    }
    
    EndBatchDraw(); // 结束批量绘图

    // 游戏开始时只播放背景音乐，移除角色语音
    PlayBackgroundMusic();  // 只播放背景音乐

    // 加载白天和黑夜背景图片（支持多关卡）
    IMAGE daytime, evening;
    IMAGE daytime2, evening2;  // 第二关背景
    loadimage(&daytime, "D:\\Cyber-Space\\c++\\Gemshin-games\\pictures\\daytime.png", width, height);
    loadimage(&evening, "D:\\Cyber-Space\\c++\\Gemshin-games\\pictures\\evening.png", width, height);
    loadimage(&daytime2, "D:\\Cyber-Space\\c++\\Gemshin-games\\pictures\\2\\daytime.png", width, height);
    loadimage(&evening2, "D:\\Cyber-Space\\c++\\Gemshin-games\\pictures\\2\\evening.png", width, height);

    // 加载角色图片和对应的掩码图片
    IMAGE characterLeft, characterLeftMask;
    IMAGE characterRight, characterRightMask;
    IMAGE niaoju, niaoujuMask;
    IMAGE tofu, tofuMask;

    // 加载第二关专用的游戏元素
    IMAGE gouyu, gouyuMask;
    IMAGE jiguanDay, jiguanNight;
    
    // 加载原始图像
    loadimage(&characterLeft, "D:\\Cyber-Space\\c++\\Gemshin-games\\pictures\\aqiao-l.png", 69, 69);
    loadimage(&characterRight, "D:\\Cyber-Space\\c++\\Gemshin-games\\pictures\\aqiao-r.png", 69, 69);
    loadimage(&niaoju, "D:\\Cyber-Space\\c++\\Gemshin-games\\pictures\\niaoju.png", 50, 50);
    loadimage(&tofu, "D:\\Cyber-Space\\c++\\Gemshin-games\\pictures\\tofu.png", 50, 50);
    loadimage(&gouyu, "D:\\Cyber-Space\\c++\\Gemshin-games\\pictures\\gouyu.png", 30, 30);
    
    // 加载掩码图像
    loadimage(&characterLeftMask, "D:\\Cyber-Space\\c++\\Gemshin-games\\pictures\\aqiao-l-mask.png", 69, 69);
    loadimage(&characterRightMask, "D:\\Cyber-Space\\c++\\Gemshin-games\\pictures\\aqiao-r-mask.png", 69, 69);
    loadimage(&niaoujuMask, "D:\\Cyber-Space\\c++\\Gemshin-games\\pictures\\niaoju-mask.png", 50, 50);
    loadimage(&tofuMask, "D:\\Cyber-Space\\c++\\Gemshin-games\\pictures\\tofu-mask.png", 50, 50);
    loadimage(&gouyuMask, "D:\\Cyber-Space\\c++\\Gemshin-games\\pictures\\gouyu-night-mask.png", 30, 30);

    // 加载蘑菇怪图片
    IMAGE moguLeft, moguLeftMask, moguRight, moguRightMask;
    loadimage(&moguLeft, "D:\\Cyber-Space\\c++\\Gemshin-games\\pictures\\mogu-l.png", 50, 50); // 假设蘑菇怪大小为50x50
    loadimage(&moguLeftMask, "D:\\Cyber-Space\\c++\\Gemshin-games\\pictures\\mogu-l-mask.png", 50, 50);
    loadimage(&moguRight, "D:\\Cyber-Space\\c++\\Gemshin-games\\pictures\\mogu-r.png", 50, 50);
    loadimage(&moguRightMask, "D:\\Cyber-Space\\c++\\Gemshin-games\\pictures\\mogu-r-mask.png", 50, 50);

    // 加载乌鸦图片
    IMAGE wuyaDaytime, wuyaDaytimeMask;
    IMAGE wuyaNightLeft, wuyaNightLeftMask, wuyaNightRight, wuyaNightRightMask;
    loadimage(&wuyaDaytime, "D:\\Cyber-Space\\c++\\Gemshin-games\\pictures\\wuya-daytime.png", 50, 50);
    loadimage(&wuyaDaytimeMask, "D:\\Cyber-Space\\c++\\Gemshin-games\\pictures\\wuya--daytime-mask.png", 50, 50);
    loadimage(&wuyaNightLeft, "D:\\Cyber-Space\\c++\\Gemshin-games\\pictures\\wuya-night-l.png", 50, 50);
    loadimage(&wuyaNightLeftMask, "D:\\Cyber-Space\\c++\\Gemshin-games\\pictures\\wuya-night-l-mask.png", 50, 50);
    loadimage(&wuyaNightRight, "D:\\Cyber-Space\\c++\\Gemshin-games\\pictures\\wuya-night-r.png", 50, 50);
    loadimage(&wuyaNightRightMask, "D:\\Cyber-Space\\c++\\Gemshin-games\\pictures\\wuya-night-r-mask.png", 50, 50);

    // 检查图像是否成功加载
    if (characterRight.getwidth() == 0 || characterRightMask.getwidth() == 0 ||
        characterLeft.getwidth() == 0 || characterLeftMask.getwidth() == 0 ||
        niaoju.getwidth() == 0 || niaoujuMask.getwidth() == 0 ||
        tofu.getwidth() == 0 || tofuMask.getwidth() == 0 ||
        gouyu.getwidth() == 0 || gouyuMask.getwidth() == 0 || // 检查勾玉图片
        moguLeft.getwidth() == 0 || moguLeftMask.getwidth() == 0 || // 检查蘑菇怪图片
        moguRight.getwidth() == 0 || moguRightMask.getwidth() == 0 ||
        wuyaDaytime.getwidth() == 0 || wuyaDaytimeMask.getwidth() == 0 || // 检查乌鸦图片
        wuyaNightLeft.getwidth() == 0 || wuyaNightLeftMask.getwidth() == 0 ||
        wuyaNightRight.getwidth() == 0 || wuyaNightRightMask.getwidth() == 0) {
        std::cout << "图像加载失败！请检查路径。" << std::endl;
        closegraph();
        return 1;
    }

    // 添加关卡系统
    int currentLevel = 1;       // 当前关卡
    bool isDaytime = true;
    bool hasEatenTofu = false;  // 添加变量跟踪豆腐是否被吃掉
    bool hasEatenGouyu = false; // 是否吃到勾玉（第二关）
    bool showFKey = false;      // 是否显示F键提示
    bool gateOpen = false;      // 机关门是否打开

    // 角色初始位置和状态
    int characterX = 50;
    int characterY = 340;
    int moveSpeed = 5;
    int jumpSpeed = 12;  // 跳跃初始速度
    int gravity = 1;     // 重力
    int initialY = 342; // 第一关地面高度
    int initialY2 = 390; // 第二关地面高度
    bool facingRight = true;
    bool isJumping = false;
    int jumpHeight = 0;
    bool onPlatform = false;

    // 初始化第一关的蘑菇怪
    Mogu mogu1_1 = {
        630, 399 - 50, // 初始位置在 (485, 399) 的上方，因为蘑菇怪有高度
        50, 50,       // 宽高
        2,            // 移动速度
        true,         // 初始朝向右
        630, 630 + 405, // 移动范围 (例如，在485右侧100像素内移动)
        moguLeft, moguLeftMask,
        moguRight, moguRightMask
    };

    Mogu mogu1_2 = {
        1025, 399 - 50, // 初始位置在 (1025, 399) 的上方
        50, 50,       // 宽高
        2,            // 移动速度
        false,        // 初始朝向左
        1025 - 405, 1025, // 移动范围
        moguLeft, moguLeftMask,
        moguRight, moguRightMask
    };


    // 当前关卡使用的蘑菇怪（动态切换）
    Mogu mogu, mogu2;

    // 初始化第二关的第一个乌鸦（位于小凸起2和竖台阶之间）
    Wuya wuya2_1 = {
        650, 385 - 50,  // 初始位置：x=650（在小凸起2和竖台阶之间），y=390-50（地面上方）
        40, 40,         // 宽高
        1,              // 移动速度（夜晚时）
        false,          // 初始朝向左（朝向玩家方向）
        550, 750,       // 移动范围（在小凸起2和竖台阶之间的区域）
        true,           // 初始为中立状态（白天）
        wuyaDaytime, wuyaDaytimeMask,           // 白天图片
        wuyaNightLeft, wuyaNightLeftMask,      // 夜晚左向图片
        wuyaNightRight, wuyaNightRightMask     // 夜晚右向图片
    };

    // 初始化第二关的第二个乌鸦（位于第三个凸起右侧）
    Wuya wuya2_2 = {
        750, 240 - 50,  // 初始位置：x=750（在第三个凸起右侧），y=235-50（第三个凸起上方）
        40, 40,         // 宽高
        1,              // 移动速度（夜晚时）
        true,           // 初始朝向右
        750, 1000,       // 移动范围（第三个凸起右侧区域）
        true,           // 初始为中立状态（白天）
        wuyaDaytime, wuyaDaytimeMask,           // 白天图片
        wuyaNightLeft, wuyaNightLeftMask,      // 夜晚左向图片
        wuyaNightRight, wuyaNightRightMask     // 夜晚右向图片
    };

    // 当前关卡使用的乌鸦
    Wuya wuya, wuya2;

    // 定义第一关的障碍物和区域
    std::vector<Obstacle> obstacles1 = {
        {174, 300, 47, 94, RGB(255, 192, 203)},  // 左边高墙
        {820, 320, 190, 30, RGB(255, 192, 203)}    // 右边二楼台阶
    };
    std::vector<DeathZone> deathZones1 = {
        {316, 424, 50, 25},   // 第一个坑
        {485, 424, 145, 25}    // 第二个坑
    };
    std::vector<TransparentZone> transparentZones1 = {
        {316, 399, 50, 25},   // 第一个坑上方的透明区域
        {485, 399, 145, 25}    // 第二个坑上方的透明区域
    };

    // 定义第二关的障碍物和区域
    std::vector<Obstacle> obstacles2_daytime = {
        {149, 418, 25, 27, RGB(255, 192, 203)},  // 小凸起（白天黑夜都存在）
        {127, 275, 95, 23, RGB(255, 192, 203)},  // 台阶2：位置(127,275)，长度95，宽度23（只白天存在）
        {56, 200, 100, 14, RGB(255, 192, 203)},  // 台阶3：位置(56,200)，长度100，宽度14（只白天存在）
        {525, 380, 360, 10, RGB(255, 192, 203)}, // 右侧台阶（白天黑夜都存在）
        {810, 380, 200, 10, RGB(255, 192, 203)}, // 右侧台阶（只白天存在）
        {770, 352, 25, 27, RGB(255, 192, 203)},  // 小凸起2（白天黑夜都存在）
        {700, 235, 25, 27, RGB(255, 192, 203)},  // 小凸起3（白天黑夜都存在）
        {525, 285, 29, 94, RGB(255, 192, 203)}, // 右侧竖台阶（白天黑夜都存在）
        {490, 285, 65, 10, RGB(255, 192, 203)}, // 右侧台阶2（只白天存在）
        {652, 260, 355, 10, RGB(255, 192, 203)}, // 右侧台阶3（白天黑夜都存在）
        {895, 150, 120, 15, RGB(255, 192, 203)}, // 右侧台阶4（白天黑夜都存在）
        {896, 80, 15, 85, RGB(255, 192, 203)}, // 鸟居左侧竖障碍（白天黑夜都存在，需要勾玉才能通过）
    };
    std::vector<Obstacle> obstacles2_nighttime = {
        {149, 418, 25, 27, RGB(255, 192, 203)},  // 小凸起（白天黑夜都存在）
        {770, 352, 25, 27, RGB(255, 192, 203)},  // 小凸起2（白天黑夜都存在）
        {700, 235, 25, 27, RGB(255, 192, 203)},  // 小凸起3（白天黑夜都存在）
        {525, 380, 360, 10, RGB(255, 192, 203)}, // 右侧台阶（白天黑夜都存在）
        {525, 285, 29, 94, RGB(255, 192, 203)}, // 右侧竖台阶（白天黑夜都存在）
        {490, 285, 65, 10, RGB(255, 192, 203)}, // 右侧台阶2（白天黑夜都存在）
        {652, 260, 355, 10, RGB(255, 192, 203)}, // 右侧台阶3（白天黑夜都存在）
        {895, 150, 120, 15, RGB(255, 192, 203)}, // 右侧台阶4（白天黑夜都存在）
        {896, 80, 15, 85, RGB(255, 192, 203)}, // 鸟居左侧竖障碍（白天黑夜都存在，需要勾玉才能通过）
        {220, 337, 95, 23, RGB(255, 192, 203)},  // 台阶1只黑夜存在）
    };
    std::vector<DeathZone> deathZones2 = {
        // 第二关暂时没有死亡区域
    };
    std::vector<TransparentZone> transparentZones2 = {
        // 第二关暂时没有透明区域
    };

    // 当前关卡使用的障碍物和区域（动态切换）
    std::vector<Obstacle> obstacles;
    std::vector<DeathZone> deathZones;
    std::vector<TransparentZone> transparentZones;

    // 定义重生点
    int respawnX = 50;
    int respawnY = 340;

    // 绘制初始背景和角色
    putimage(0, 0, &daytime);
    
    // 使用SRCAND和SRCPAINT组合实现透明效果
    putimage(characterX, characterY, &characterRightMask, SRCAND);
    putimage(characterX, characterY, &characterRight, SRCPAINT);

    // 初始化游戏状态
    bool gameOver = false;
    bool needRestart = false;

    // 根据当前关卡和时间设置障碍物和区域的函数
    auto updateLevelData = [&]() {
        if (currentLevel == 1) {
            obstacles = obstacles1;
            deathZones = deathZones1;
            transparentZones = transparentZones1;
            mogu = mogu1_1;
            mogu2 = mogu1_2;
        } else if (currentLevel == 2) {
            // 第二关根据白天/黑夜设置不同的障碍物
            if (isDaytime) {
                obstacles = obstacles2_daytime;    // 白天：小凸起 + 台阶2 + 台阶3
            } else {
                obstacles = obstacles2_nighttime;  // 黑夜：小凸起 + 台阶1
            }

            // 如果已经吃了勾玉，移除竖障碍
            if (hasEatenGouyu) {
                obstacles.erase(
                    std::remove_if(obstacles.begin(), obstacles.end(),
                        [](const Obstacle& obs) {
                            return obs.x == 896 && obs.y == 80; // 竖障碍的位置
                        }),
                    obstacles.end()
                );
            }

            deathZones = deathZones2;
            transparentZones = transparentZones2;
            // 第二关有两个乌鸦
            // 只在第一次进入第二关时初始化乌鸦位置
            static bool wuyaInitialized = false;
            if (!wuyaInitialized) {
                wuya = wuya2_1;   // 第一个乌鸦
                wuya2 = wuya2_2;  // 第二个乌鸦
                wuyaInitialized = true;
            }
            wuya.isNeutral = isDaytime;   // 第一个乌鸦：白天中立，夜晚敌对
            wuya2.isNeutral = isDaytime;  // 第二个乌鸦：白天中立，夜晚敌对
            // 不重置位置，保持乌鸦当前位置
        }
    };

    // 初始化第一关的数据
    updateLevelData();

    // 消息处理循环
    ExMessage msg;
    BeginBatchDraw();  // 开始批量绘制
    while (true) {
        if (gameOver) {
            // 游戏结束处理
            mciSendString("close all", nullptr, 0, nullptr);
            cleardevice();
            putimage(0, 0, &evening);
            
            // 显示游戏结束文字（使用宽字符以正确显示中文）
            settextcolor(WHITE);
            LOGFONT font;
            gettextstyle(&font);
            font.lfHeight = 40;
            _tcscpy(font.lfFaceName, _T("微软雅黑"));
            font.lfQuality = ANTIALIASED_QUALITY;
            settextstyle(&font);
            setbkmode(TRANSPARENT);
            RECT textRect = {width/2 - 100, height/2 - 100, width/2 + 100, height/2 - 60};
            drawtext(_T("GAME OVER!"), &textRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            
            // 绘制重新开始按钮
            bool buttonHovered = false;
            int buttonSize = 30;
            int buttonX_restart = width/2;
            int buttonY_restart = height/2;
            
            while (true) {
                ExMessage msg_restart;
                if (peekmessage(&msg_restart, EM_MOUSE)) {
                    int dx = msg_restart.x - buttonX_restart;
                    int dy = msg_restart.y - buttonY_restart;
                    buttonHovered = (dx * dx + dy * dy <= buttonSize * buttonSize);
                    
                    if (buttonHovered && msg_restart.message == WM_LBUTTONDOWN) {
                        // 重置游戏状态
                        lives = 3;
                        characterX = 50;
                        characterY = 340;
                        facingRight = true;
                        isJumping = false;
                        jumpHeight = 0;
                        hasEatenTofu = false;
                        isDaytime = true;
                        isInvincible = false;
                        gameOver = false;
                        PlayBackgroundMusic();
                        break;
                    }
                }
                
                DrawRestartButton(buttonX_restart, buttonY_restart, buttonSize, buttonHovered);
                FlushBatchDraw();
                
                if (GetAsyncKeyState(VK_ESCAPE)) {
                    return 0;
                }
                
                Sleep(10);
            }
            continue;
        }

        // 清空画面并重绘背景（根据关卡选择背景）
        cleardevice();
        if (currentLevel == 1) {
            if (isDaytime) {
                putimage(0, 0, &daytime);
            } else {
                putimage(0, 0, &evening);
            }
        } else if (currentLevel == 2) {
            if (isDaytime) {
                putimage(0, 0, &daytime2);
            } else {
                putimage(0, 0, &evening2);
            }
        }

        // 获取鼠标消息（用于切换背景）
        if (peekmessage(&msg, EX_MOUSE)) {
            // 检测鼠标右键点击，切换背景
            if (msg.message == WM_RBUTTONDOWN) {
                isDaytime = !isDaytime;
                // 切换到黑夜时取消所有平台碰撞状态
                if (!isDaytime) {
                    onPlatform = false;
                }
                // 第二关需要根据白天/黑夜更新障碍物
                if (currentLevel == 2) {
                    updateLevelData();
                }
            }
        }

        // 处理跳跃
        if (GetAsyncKeyState(VK_SPACE) & 0x8000 && !isJumping) {
            isJumping = true;
            jumpHeight = jumpSpeed;
        }

        // 全局重力检测 - 检查角色是否应该下落
        bool shouldFall = true;
        int currentGroundY = (currentLevel == 1) ? initialY : initialY2;

        // 检查是否站在障碍物上
        for (const auto& obstacle : obstacles) {
            if (characterX + 60 > obstacle.x && characterX + 10 < obstacle.x + obstacle.width) {
                if (characterY + 69 >= obstacle.y && characterY + 69 <= obstacle.y + 5 && characterY < obstacle.y) {
                    shouldFall = false;
                    break;
                }
            }
        }

        // 检查是否在地面上
        if (characterY >= currentGroundY) {
            shouldFall = false;
        }

        // 第一关黑夜模式的特殊处理：检查透明区域
        if (currentLevel == 1 && !isDaytime) {
            bool inTransparentZone = false;
            for (const auto& zone : transparentZones) {
                int characterCenterX = characterX + 35;
                if (characterCenterX > zone.x && characterCenterX < zone.x + zone.width &&
                    characterY + 69 > zone.y) {
                    inTransparentZone = true;
                    shouldFall = true; // 在透明区域强制下落
                    break;
                }
            }
        }

        // 如果应该下落且不在跳跃状态，开始下落
        if (shouldFall && !isJumping) {
            isJumping = true;
            jumpHeight = -1; // 给一个小的向下初速度
        }

        // 更新角色位置
        if (isJumping) {
            int nextY = characterY - jumpHeight;  // 计算下一帧的位置

            // 白天和第二关黑夜都进行碰撞检测
            if (isDaytime || (!isDaytime && currentLevel == 2)) {
                bool canLand = false;
                for (const auto& obstacle : obstacles) {
                    // 检查是否完全在平台正下方（考虑边缘区域）
                    bool fullyUnderPlatform = characterX + 30 > obstacle.x &&
                                            characterX + 50 < obstacle.x + obstacle.width;

                    // 只有完全在平台下方时才检查垂直碰撞
                    if (fullyUnderPlatform) {
                        if (nextY < obstacle.y + obstacle.height &&
                            characterY + 69 > obstacle.y) {
                            // 被平台挡住，不允许向上移动
                            nextY = characterY;
                            jumpHeight = 0;
                            break;
                        }
                    }

                    // 检查是否可以从上方着陆
                    if (characterX + 60 > obstacle.x && characterX + 10 < obstacle.x + obstacle.width) {
                        if (jumpHeight < 0) {  // 下落时
                            if (nextY + 69 > obstacle.y && characterY + 69 <= obstacle.y + 5) {
                                nextY = obstacle.y - 69;
                                isJumping = false;
                                jumpHeight = 0;
                                canLand = true;
                                break;
                            }
                        }
                    }
                }
            }

            // 更新位置
            characterY = nextY;
            jumpHeight -= gravity;  // 应用重力
        }

        // 碰撞检测
        if (isDaytime) {
            // 白天模式的碰撞检测
            bool platformCollision = false;
            
            for (const auto& obstacle : obstacles) {
                // 水平方向碰撞检测（根据关卡区分处理）
                if (currentLevel == 1) {
                    // 第一关：只检测墙壁（高度大于30的障碍物）
                    if (obstacle.height > 30) {
                        if (characterX + 69 > obstacle.x && characterX < obstacle.x + obstacle.width) {
                            // 左侧碰撞
                            if (characterX + 69 > obstacle.x && characterX + 69 < obstacle.x + 20) {
                                characterX = obstacle.x - 69;
                            }
                            // 右侧碰撞
                            if (characterX < obstacle.x + obstacle.width && characterX > obstacle.x + obstacle.width - 20) {
                                characterX = obstacle.x + obstacle.width;
                            }
                        }
                    }
                } else if (currentLevel == 2) {
                    // 第二关：统一的碰撞检测逻辑
                    // 特殊处理：鸟居左侧竖障碍（需要勾玉才能通过）
                    if (obstacle.x == 896 && obstacle.y == 80 && !hasEatenGouyu) {
                        // 这是鸟居左侧的竖障碍，且还没吃勾玉，正常碰撞
                        bool horizontalOverlap = characterX + 69 > obstacle.x && characterX < obstacle.x + obstacle.width;
                        bool verticalOverlap = characterY + 69 > obstacle.y && characterY < obstacle.y + obstacle.height;

                        if (horizontalOverlap && verticalOverlap) {
                            // 计算各个方向的重叠距离
                            int overlapLeft = characterX + 69 - obstacle.x;
                            int overlapRight = obstacle.x + obstacle.width - characterX;
                            int overlapTop = characterY + 69 - obstacle.y;
                            int overlapBottom = obstacle.y + obstacle.height - characterY;

                            // 找到最小的重叠距离，决定推出方向
                            int minOverlap = overlapLeft;
                            if (overlapRight < minOverlap) minOverlap = overlapRight;
                            if (overlapTop < minOverlap) minOverlap = overlapTop;
                            if (overlapBottom < minOverlap) minOverlap = overlapBottom;

                            if (minOverlap == overlapLeft && overlapLeft < 20) {
                                characterX = obstacle.x - 69;
                            } else if (minOverlap == overlapRight && overlapRight < 20) {
                                characterX = obstacle.x + obstacle.width;
                            }
                        }
                    } else if (obstacle.x == 896 && obstacle.y == 80 && hasEatenGouyu) {
                        // 这是鸟居左侧的竖障碍，但已经吃了勾玉，可以穿过，跳过碰撞检测
                        continue;
                    } else {
                        // 其他障碍物的正常碰撞检测
                        bool horizontalOverlap = characterX + 69 > obstacle.x && characterX < obstacle.x + obstacle.width;
                        bool verticalOverlap = characterY + 69 > obstacle.y && characterY < obstacle.y + obstacle.height;

                        if (horizontalOverlap && verticalOverlap) {
                            // 计算各个方向的重叠距离
                            int overlapLeft = characterX + 69 - obstacle.x;
                            int overlapRight = obstacle.x + obstacle.width - characterX;
                            int overlapTop = characterY + 69 - obstacle.y;
                            int overlapBottom = obstacle.y + obstacle.height - characterY;

                            // 找到最小的重叠距离，决定推出方向
                            int minOverlap = overlapLeft;
                            if (overlapRight < minOverlap) minOverlap = overlapRight;
                            if (overlapTop < minOverlap) minOverlap = overlapTop;
                            if (overlapBottom < minOverlap) minOverlap = overlapBottom;

                            if (minOverlap == overlapLeft && overlapLeft < 20) {
                                // 从左侧推出
                                characterX = obstacle.x - 69;
                            } else if (minOverlap == overlapRight && overlapRight < 20) {
                                // 从右侧推出
                                characterX = obstacle.x + obstacle.width;
                            } else if (minOverlap == overlapTop && overlapTop < 20) {
                                // 从上方推出（站在平台上）
                                characterY = obstacle.y - 69;
                                if (jumpHeight < 0) {
                                    isJumping = false;
                                    jumpHeight = 0;
                                    platformCollision = true;
                                }
                            } else if (minOverlap == overlapBottom && overlapBottom < 20) {
                                // 从下方推出（撞到平台底部）
                                characterY = obstacle.y + obstacle.height;
                                if (jumpHeight > 0) {
                                    jumpHeight = 0;
                                }
                            }
                        }
                    }

                    // 检查是否站在平台上（从上方接触）
                    if (characterX + 60 > obstacle.x && characterX + 10 < obstacle.x + obstacle.width) {
                        if (characterY + 69 >= obstacle.y && characterY + 69 <= obstacle.y + 5 && characterY < obstacle.y) {
                            platformCollision = true;
                            characterY = obstacle.y - 69;
                            if (jumpHeight < 0) {
                                isJumping = false;
                                jumpHeight = 0;
                            }
                        }
                    }
                }

                // 第一关的平台检测（保持原有逻辑）
                if (currentLevel == 1) {
                    if (characterX + 60 > obstacle.x && characterX + 10 < obstacle.x + obstacle.width) {
                        if (characterY + 69 >= obstacle.y && characterY + 69 <= obstacle.y + 5 && characterY < obstacle.y) {
                            platformCollision = true;
                            characterY = obstacle.y - 69;
                            if (jumpHeight < 0) {  // 只有在下落时才停止跳跃
                                isJumping = false;
                                jumpHeight = 0;
                            }
                            break;
                        }
                    }
                }
            }

            // 更新平台状态
            onPlatform = platformCollision;

            // 添加地面碰撞检测（根据关卡使用不同地面高度）
            int currentGroundY = (currentLevel == 1) ? initialY : initialY2;
            if (!onPlatform && characterY >= currentGroundY) {
                characterY = currentGroundY;
                isJumping = false;
                jumpHeight = 0;
                onPlatform = true;
            }
        } else {
            // 黑夜模式：检测是否在透明区域
            bool inTransparentZone = false;
            for (const auto& zone : transparentZones) {
                // 修改检测逻辑：只要角色中心点在透明区域的水平范围内就算作在透明区域上
                int characterCenterX = characterX + 35;
                if (characterCenterX > zone.x && characterCenterX < zone.x + zone.width &&
                    characterY + 69 > zone.y) {
                    inTransparentZone = true;
                    break;
                }
            }

            // 如果在透明区域，强制下落（不受地面限制）
            if (inTransparentZone) {
                onPlatform = false;
                if (!isJumping) {
                    isJumping = true;
                    jumpHeight = -2; // 给一个小的向下的初速度，使下落更自然
                }
            } else {
                // 不在透明区域时，检查障碍物碰撞和地面
                bool platformCollision = false;

                // 第二关在黑夜模式下也需要检查障碍物碰撞
                if (currentLevel == 2) {
                    for (const auto& obstacle : obstacles) {
                        // 第二关：统一的碰撞检测逻辑（与白天相同）
                        // 特殊处理：鸟居左侧竖障碍（需要勾玉才能通过）
                        if (obstacle.x == 896 && obstacle.y == 80 && !hasEatenGouyu) {
                            // 这是鸟居左侧的竖障碍，且还没吃勾玉，正常碰撞
                            bool horizontalOverlap = characterX + 69 > obstacle.x && characterX < obstacle.x + obstacle.width;
                            bool verticalOverlap = characterY + 69 > obstacle.y && characterY < obstacle.y + obstacle.height;

                            if (horizontalOverlap && verticalOverlap) {
                                // 计算各个方向的重叠距离
                                int overlapLeft = characterX + 69 - obstacle.x;
                                int overlapRight = obstacle.x + obstacle.width - characterX;
                                int overlapTop = characterY + 69 - obstacle.y;
                                int overlapBottom = obstacle.y + obstacle.height - characterY;

                                // 找到最小的重叠距离，决定推出方向
                                int minOverlap = overlapLeft;
                                if (overlapRight < minOverlap) minOverlap = overlapRight;
                                if (overlapTop < minOverlap) minOverlap = overlapTop;
                                if (overlapBottom < minOverlap) minOverlap = overlapBottom;

                                if (minOverlap == overlapLeft && overlapLeft < 20) {
                                    characterX = obstacle.x - 69;
                                } else if (minOverlap == overlapRight && overlapRight < 20) {
                                    characterX = obstacle.x + obstacle.width;
                                }
                            }
                        } else if (obstacle.x == 896 && obstacle.y == 80 && hasEatenGouyu) {
                            // 这是鸟居左侧的竖障碍，但已经吃了勾玉，可以穿过，跳过碰撞检测
                            continue;
                        } else {
                            // 其他障碍物的正常碰撞检测
                            bool horizontalOverlap = characterX + 69 > obstacle.x && characterX < obstacle.x + obstacle.width;
                            bool verticalOverlap = characterY + 69 > obstacle.y && characterY < obstacle.y + obstacle.height;

                            if (horizontalOverlap && verticalOverlap) {
                                // 计算各个方向的重叠距离
                                int overlapLeft = characterX + 69 - obstacle.x;
                                int overlapRight = obstacle.x + obstacle.width - characterX;
                                int overlapTop = characterY + 69 - obstacle.y;
                                int overlapBottom = obstacle.y + obstacle.height - characterY;

                                // 找到最小的重叠距离，决定推出方向
                                int minOverlap = overlapLeft;
                                if (overlapRight < minOverlap) minOverlap = overlapRight;
                                if (overlapTop < minOverlap) minOverlap = overlapTop;
                                if (overlapBottom < minOverlap) minOverlap = overlapBottom;

                                if (minOverlap == overlapLeft && overlapLeft < 20) {
                                    // 从左侧推出
                                    characterX = obstacle.x - 69;
                                } else if (minOverlap == overlapRight && overlapRight < 20) {
                                    // 从右侧推出
                                    characterX = obstacle.x + obstacle.width;
                                } else if (minOverlap == overlapTop && overlapTop < 20) {
                                    // 从上方推出（站在平台上）
                                    characterY = obstacle.y - 69;
                                    if (jumpHeight < 0) {
                                        isJumping = false;
                                        jumpHeight = 0;
                                        platformCollision = true;
                                    }
                                } else if (minOverlap == overlapBottom && overlapBottom < 20) {
                                    // 从下方推出（撞到平台底部）
                                    characterY = obstacle.y + obstacle.height;
                                    if (jumpHeight > 0) {
                                        jumpHeight = 0;
                                    }
                                }
                            }
                        }

                        // 检查是否站在平台上（从上方接触）
                        if (characterX + 60 > obstacle.x && characterX + 10 < obstacle.x + obstacle.width) {
                            if (characterY + 69 >= obstacle.y && characterY + 69 <= obstacle.y + 5 && characterY < obstacle.y) {
                                platformCollision = true;
                                characterY = obstacle.y - 69;
                                if (jumpHeight < 0) {
                                    isJumping = false;
                                    jumpHeight = 0;
                                }
                            }
                        }
                    }
                }

                // 检查是否在地面（根据关卡使用不同地面高度）
                int currentGroundY = (currentLevel == 1) ? initialY : initialY2;
                if (!platformCollision && characterY >= currentGroundY) {
                    characterY = currentGroundY;
                    isJumping = false;
                    jumpHeight = 0;
                    onPlatform = true;
                } else if (!platformCollision && !isJumping) {
                    isJumping = true;
                    jumpHeight = 0;
                }

                // 更新平台状态
                if (platformCollision) {
                    onPlatform = true;
                }
            }

            // 在这里进行死亡检测，作为物理更新的一部分
            if (!isInvincible && !isDaytime) {  // 只在夜晚模式且非无敌状态下检查
                for (const auto& zone : deathZones) {
                    // 计算角色的中心点
                    int characterCenterX = characterX + 35;
                    int characterCenterY = characterY + 35;
                    
                    // 检查角色中心点是否在死亡区域内
                    if (characterCenterX > zone.x && 
                        characterCenterX < zone.x + zone.width &&
                        characterCenterY > zone.y && 
                        characterCenterY < zone.y + zone.height) {
                        
                        // 角色死亡
                        lives--;
                        if (lives > 0) {
                            // 重生时设置无敌时间
                            isInvincible = true;
                            invincibleTimer = INVINCIBLE_DURATION;
                            
                            // 获取最近的重生点
                            int newRespawnX, newRespawnY;
                            GetNearestRespawnPoint(characterX, characterY, facingRight, newRespawnX, newRespawnY, currentLevel);
                            characterX = newRespawnX;
                            characterY = newRespawnY;
                            facingRight = !facingRight;  // 重生后朝向相反
                            isJumping = false;
                            jumpHeight = 0;
                            
                            // 播放重生语音
                            PlayCharacterVoice();
                            // 确保背景音乐仍在播放
                            char status[256];
                            mciSendString("status bgm mode", status, sizeof(status), nullptr);
                            if (strcmp(status, "playing") != 0) {
                                PlayBackgroundMusic();
                            }
                            continue;  // 跳过当前帧的剩余处理
                        } else {
                            gameOver = true;
                            continue;
                        }
                    }
                }
            }

            // 蘑菇怪碰撞检测 (只在黑夜且非无敌状态且第一关)
            if (!isInvincible && !isDaytime && currentLevel == 1) {
                // 检测与第一个蘑菇怪的碰撞
                if (characterX < mogu.x + mogu.width &&
                    characterX + 69 > mogu.x &&
                    characterY < mogu.y + mogu.height &&
                    characterY + 69 > mogu.y) {
                    
                    lives--;
                    if (lives > 0) {
                        isInvincible = true;
                        invincibleTimer = INVINCIBLE_DURATION;
                        int newRespawnX, newRespawnY;
                        GetNearestRespawnPoint(characterX, characterY, facingRight, newRespawnX, newRespawnY, currentLevel);
                        characterX = newRespawnX;
                        characterY = newRespawnY;
                        facingRight = !facingRight;
                        isJumping = false;
                        jumpHeight = 0;
                        PlayCharacterVoice();
                        char status[256];
                        mciSendString("status bgm mode", status, sizeof(status), nullptr);
                        if (strcmp(status, "playing") != 0) {
                            PlayBackgroundMusic();
                        }
                    } else {
                        gameOver = true;
                        continue;
                    }
                }

                // 检测与第二个蘑菇怪的碰撞
                if (characterX < mogu2.x + mogu2.width &&
                    characterX + 69 > mogu2.x &&
                    characterY < mogu2.y + mogu2.height &&
                    characterY + 69 > mogu2.y) {
                    
                    lives--;
                    if (lives > 0) {
                        isInvincible = true;
                        invincibleTimer = INVINCIBLE_DURATION;
                        int newRespawnX, newRespawnY;
                        GetNearestRespawnPoint(characterX, characterY, facingRight, newRespawnX, newRespawnY, currentLevel);
                        characterX = newRespawnX;
                        characterY = newRespawnY;
                        facingRight = !facingRight;
                        isJumping = false;
                        jumpHeight = 0;
                        PlayCharacterVoice();
                        char status[256];
                        mciSendString("status bgm mode", status, sizeof(status), nullptr);
                        if (strcmp(status, "playing") != 0) {
                            PlayBackgroundMusic();
                        }
                    } else {
                        gameOver = true;
                        continue;
                    }
                }
            }
        }

        // 更新无敌时间
        if (isInvincible) {
            invincibleTimer--;
            if (invincibleTimer <= 0) {
                isInvincible = false;
            }
        }
        
        // 检查F1键状态（切换调试模式）
        if (GetAsyncKeyState(VK_F1) & 0x8000) {
            if (!F1Pressed) {  // 防止持续按住时重复切换
                showObstacles = !showObstacles;
                F1Pressed = true;
            }
        } else {
            F1Pressed = false;
        }

        // 在调试模式下显示障碍物和死亡区域
        if (showObstacles) {
            for (const auto& obstacle : obstacles) {
                // 特殊处理：如果是鸟居左侧竖障碍且已经吃了勾玉，则不显示
                if (currentLevel == 2 && obstacle.x == 896 && obstacle.y == 80 && hasEatenGouyu) {
                    continue; // 跳过绘制这个障碍物
                }

                setfillcolor(obstacle.color);
                solidrectangle(obstacle.x, obstacle.y, obstacle.x + obstacle.width, obstacle.y + obstacle.height);

                // 显示障碍物信息
                settextcolor(RGB(255, 255, 255));
                setbkmode(TRANSPARENT);
                TCHAR obstacleInfo[100];
                _stprintf(obstacleInfo, _T("H:%d"), obstacle.height);
                outtextxy(obstacle.x, obstacle.y - 15, obstacleInfo);
            }

            // 显示角色位置信息
            settextcolor(RGB(255, 255, 0));
            setbkmode(TRANSPARENT);
            TCHAR characterInfo[100];
            _stprintf(characterInfo, _T("X:%d Y:%d"), characterX, characterY);
            outtextxy(characterX, characterY - 30, characterInfo);

            // 显示死亡区域和透明区域
            if (!isDaytime) {
                // 显示死亡区域（红色）
                setfillcolor(RGB(255, 0, 0));
                for (const auto& zone : deathZones) {
                    solidrectangle(zone.x, zone.y, zone.x + zone.width, zone.y + zone.height);
                }

                // 显示透明区域（半透明的蓝色）
                setfillcolor(RGB(0, 255, 255));
                for (const auto& zone : transparentZones) {
                    solidrectangle(zone.x, zone.y, zone.x + zone.width, zone.y + zone.height);
                }
            }
        }
        
        // 绘制生命值（心形图标）
        int heartStartX = width / 2 - (3 * 40) / 2;  // 计算使心形图标居中的起始X坐标
        for (int i = 0; i < 3; i++) {
            if (i < lives) {
                putimage(heartStartX + i * 40, 20, &heartMask, SRCAND);
                putimage(heartStartX + i * 40, 20, &heartFull, SRCPAINT);
            } else {
                putimage(heartStartX + i * 40, 20, &heartMask, SRCAND);
                putimage(heartStartX + i * 40, 20, &heartEmpty, SRCPAINT);
            }
        }
        
        // 根据关卡绘制不同的游戏元素
        if (currentLevel == 1) {
            // 第一关：鸟居和豆腐系统
            if (isDaytime) {
                // 在白天模式下显示鸟居（在二楼台阶上的右侧）
                putimage(820 + 140, 320 - 50, &niaoujuMask, SRCAND);  // 放在台阶右侧
                putimage(820 + 140, 320 - 50, &niaoju, SRCPAINT);

                // 如果豆腐已经被吃掉，检查是否在鸟居附近
                if (hasEatenTofu) {
                    // 检查角色是否在鸟居附近
                    if (characterX + 69 > 820 + 140 - 30 && characterX < 820 + 140 + 80 &&
                        characterY + 69 > 320 - 50 - 30 && characterY < 320 - 50 + 80) {
                        // 显示F键提示
                        showFKey = true;
                    } else {
                        showFKey = false;
                    }

                    // 如果显示F键且按下F键
                    if (showFKey && GetAsyncKeyState('F') & 0x8000) {
                        // 第一关通关，进入第二关
                        currentLevel = 2;
                        characterX = 53;  // 第二关角色初始位置
                        characterY = 445;
                        facingRight = true;
                        isJumping = false;
                        jumpHeight = 0;
                        hasEatenTofu = false;
                        hasEatenGouyu = false;  // 重置第二关的收集状态
                        gateOpen = false;       // 重置机关门状态
                        isDaytime = true;       // 重置为白天
                        showFKey = false;
                        updateLevelData();      // 更新关卡数据
                        MessageBoxW(GetHWnd(), L"进入第二关！", L"提示", MB_OK);
                    }
                }
            } else {
                // 在黑夜模式下显示豆腐（如果还没有被吃掉）
                if (!hasEatenTofu) {
                    putimage(820 + 140, 320 + 30, &tofuMask, SRCAND);  // 放在台阶下方
                    putimage(820 + 140, 320 + 30, &tofu, SRCPAINT);

                    // 检查角色是否接触到豆腐
                    if (characterX + 69 > 820 + 140 - 20 && characterX < 820 + 140 + 70 &&
                        characterY + 69 > 320 + 30 - 20 && characterY < 320 + 30 + 70) {
                        hasEatenTofu = true;  // 豆腐被吃掉
                    }
                }
                // 在黑夜模式下强制关闭F键显示
                showFKey = false;
            }
        } else if (currentLevel == 2) {
            // 第二关：勾玉和豆腐系统
            if (isDaytime) {
                // 第二关白天模式：显示豆腐在第三台阶上
                if (!hasEatenTofu) {
                    putimage(71, 131, &tofuMask, SRCAND);  // 放在台阶3上方
                    putimage(71, 131, &tofu, SRCPAINT);

                    // 检查角色是否接触到豆腐
                    if (characterX + 69 > 71 - 20 && characterX < 71 + 70 &&
                        characterY + 69 > 131 - 20 && characterY < 131 + 70) {
                        hasEatenTofu = true;  // 豆腐被吃掉
                    }
                }

                // 第二关白天模式：显示鸟居在台阶4右侧
                putimage(925, 81, &niaoujuMask, SRCAND);  // 放在台阶4右侧
                putimage(925, 81, &niaoju, SRCPAINT);

                // 通关条件：只需要吃了豆腐，在鸟居附近按F键
                if (hasEatenTofu) {
                    // 检查角色是否在鸟居附近
                    if (characterX + 69 > 925 - 30 && characterX < 925 + 80 &&
                        characterY + 69 > 81 - 30 && characterY < 81 + 80) {
                        showFKey = true;
                        if (GetAsyncKeyState('F') & 0x8000) {
                            MessageBoxW(GetHWnd(), L"恭喜通关全部关卡！", L"提示", MB_OK);
                            return 0;
                        }
                    } else {
                        showFKey = false;
                    }
                } else {
                    showFKey = false;
                }
            } else {
                // 第二关夜晚模式：显示勾玉在台阶3上面（和豆腐同一位置）
                if (!hasEatenGouyu) {
                    putimage(500, 216, &gouyuMask, SRCAND);  // 放在台阶3上方（和豆腐同一位置）
                    putimage(500, 216, &gouyu, SRCPAINT);

                    // 检查角色是否接触到勾玉（和豆腐相同的检测方式）
                    if (characterX + 69 > 500 - 20 && characterX < 500 + 70 &&
                        characterY + 69 > 216 - 20 && characterY < 216 + 70) {
                        hasEatenGouyu = true;  // 勾玉被吃掉
                    }
                }
                showFKey = false;
            }
        }

        // 如果需要显示F键提示（只在白天模式下）
        if (showFKey && isDaytime) {
            settextcolor(RGB(255, 255, 255));  // 白色文字
            setbkmode(TRANSPARENT);            // 设置文字背景透明

            if (currentLevel == 1) {
                // 第一关：在鸟居上方显示F键
                outtextxy(820 + 140 + 15, 320 - 70, _T("F"));
            } else if (currentLevel == 2) {
                // 第二关：在鸟居上方显示F键
                outtextxy(910 + 15, 150 - 70, _T("F"));
            }
        }

        // 根据朝向绘制角色（在无敌状态下闪烁）
        if (!isInvincible || (invincibleTimer % 10 > 5)) {  // 无敌状态下闪烁效果
            if (facingRight) {
                putimage(characterX, characterY, &characterRightMask, SRCAND);
                putimage(characterX, characterY, &characterRight, SRCPAINT);
            } else {
                putimage(characterX, characterY, &characterLeftMask, SRCAND);
                putimage(characterX, characterY, &characterLeft, SRCPAINT);
            }
        }

        // 更新和绘制蘑菇怪 (只在黑夜且第一关)
        if (!isDaytime && currentLevel == 1) {
            // 更新第一个蘑菇怪位置
            if (mogu.facingRight) {
                mogu.x += mogu.moveSpeed;
                if (mogu.x + mogu.width > mogu.maxX) {
                    mogu.x = mogu.maxX - mogu.width;
                    mogu.facingRight = false;
                }
            } else {
                mogu.x -= mogu.moveSpeed;
                if (mogu.x < mogu.minX) {
                    mogu.x = mogu.minX;
                    mogu.facingRight = true;
                }
            }

            // 更新第二个蘑菇怪位置
            if (mogu2.facingRight) {
                mogu2.x += mogu2.moveSpeed;
                if (mogu2.x + mogu2.width > mogu2.maxX) {
                    mogu2.x = mogu2.maxX - mogu2.width;
                    mogu2.facingRight = false;
                }
            } else {
                mogu2.x -= mogu2.moveSpeed;
                if (mogu2.x < mogu2.minX) {
                    mogu2.x = mogu2.minX;
                    mogu2.facingRight = true;
                }
            }

            // 绘制第一个蘑菇怪
            if (mogu.facingRight) {
                putimage(mogu.x, mogu.y, &mogu.imgRightMask, SRCAND);
                putimage(mogu.x, mogu.y, &mogu.imgRight, SRCPAINT);
            } else {
                putimage(mogu.x, mogu.y, &mogu.imgLeftMask, SRCAND);
                putimage(mogu.x, mogu.y, &mogu.imgLeft, SRCPAINT);
            }

            // 绘制第二个蘑菇怪
            if (mogu2.facingRight) {
                putimage(mogu2.x, mogu2.y, &mogu2.imgRightMask, SRCAND);
                putimage(mogu2.x, mogu2.y, &mogu2.imgRight, SRCPAINT);
            } else {
                putimage(mogu2.x, mogu2.y, &mogu2.imgLeftMask, SRCAND);
                putimage(mogu2.x, mogu2.y, &mogu2.imgLeft, SRCPAINT);
            }
        }

        // 更新和绘制乌鸦 (只在第二关)
        if (currentLevel == 2) {
            if (isDaytime) {
                // 白天：乌鸦为中立状态，静止不动，作为障碍物平台
                putimage(wuya.x, wuya.y, &wuya.imgDaytimeMask, SRCAND);
                putimage(wuya.x, wuya.y, &wuya.imgDaytime, SRCPAINT);

                // 白天乌鸦作为障碍物的碰撞检测（类似小凸起）
                bool horizontalOverlap = characterX + 69 > wuya.x && characterX < wuya.x + wuya.width;
                bool verticalOverlap = characterY + 69 > wuya.y && characterY < wuya.y + wuya.height;

                // 检查靠近自动上去的功能
                bool nearWuya = horizontalOverlap && characterY + 69 >= wuya.y - 10 && characterY + 69 <= wuya.y + 10;
                if (nearWuya && !isJumping) {
                    // 靠近时自动上去
                    characterY = wuya.y - 69;
                    isJumping = false;
                    jumpHeight = 0;
                }

                // 正常的障碍物碰撞检测
                if (horizontalOverlap && verticalOverlap) {
                    // 计算各个方向的重叠距离
                    int overlapLeft = characterX + 69 - wuya.x;
                    int overlapRight = wuya.x + wuya.width - characterX;
                    int overlapTop = characterY + 69 - wuya.y;
                    int overlapBottom = wuya.y + wuya.height - characterY;

                    // 找到最小的重叠距离，决定推出方向
                    int minOverlap = overlapLeft;
                    if (overlapRight < minOverlap) minOverlap = overlapRight;
                    if (overlapTop < minOverlap) minOverlap = overlapTop;
                    if (overlapBottom < minOverlap) minOverlap = overlapBottom;

                    if (minOverlap == overlapLeft && overlapLeft < 20) {
                        // 从左侧推出
                        characterX = wuya.x - 69;
                    } else if (minOverlap == overlapRight && overlapRight < 20) {
                        // 从右侧推出
                        characterX = wuya.x + wuya.width;
                    } else if (minOverlap == overlapTop && overlapTop < 20) {
                        // 从上方推出（站在乌鸦上）
                        characterY = wuya.y - 69;
                        if (jumpHeight < 0) {
                            isJumping = false;
                            jumpHeight = 0;
                        }
                    } else if (minOverlap == overlapBottom && overlapBottom < 20) {
                        // 从下方推出（撞到乌鸦底部）
                        characterY = wuya.y + wuya.height;
                        if (jumpHeight > 0) {
                            jumpHeight = 0;
                        }
                    }
                }

                // 检查是否站在乌鸦上（从上方接触）
                if (characterX + 60 > wuya.x && characterX + 10 < wuya.x + wuya.width) {
                    if (characterY + 69 >= wuya.y && characterY + 69 <= wuya.y + 5 && characterY < wuya.y) {
                        characterY = wuya.y - 69;
                        if (jumpHeight < 0) {
                            isJumping = false;
                            jumpHeight = 0;
                        }
                    }
                }
            } else {
                // 夜晚：乌鸦为敌对状态
                // 检查是否在同一层（地面层）
                bool onSameLevel = (characterY + 69 >= 390 - 20 && characterY + 69 <= 390 + 20) &&
                                   (wuya.y + wuya.height >= 390 - 20 && wuya.y + wuya.height <= 390 + 20);

                // 检查是否能看到玩家（视野范围内且同一层）
                int distanceToPlayer = abs(characterX - wuya.x);
                bool canSeePlayer = onSameLevel && distanceToPlayer <= 200; // 视野范围200像素

                if (canSeePlayer) {
                    // 看到玩家时，朝玩家方向移动
                    if (characterX < wuya.x) {
                        wuya.facingRight = false;
                        wuya.x -= wuya.moveSpeed;
                        if (wuya.x < wuya.minX) {
                            wuya.x = wuya.minX;
                        }
                    } else if (characterX > wuya.x + wuya.width) {
                        wuya.facingRight = true;
                        wuya.x += wuya.moveSpeed;
                        if (wuya.x + wuya.width > wuya.maxX) {
                            wuya.x = wuya.maxX - wuya.width;
                        }
                    }
                } else {
                    // 没看到玩家时，正常巡逻
                    if (wuya.facingRight) {
                        wuya.x += wuya.moveSpeed;
                        if (wuya.x + wuya.width >= wuya.maxX) {
                            wuya.x = wuya.maxX - wuya.width;
                            wuya.facingRight = false;
                        }
                    } else {
                        wuya.x -= wuya.moveSpeed;
                        if (wuya.x <= wuya.minX) {
                            wuya.x = wuya.minX;
                            wuya.facingRight = true;
                        }
                    }
                }

                // 碰撞检测（只在同一层且非无敌状态）
                if (onSameLevel && !isInvincible) {
                    if (characterX < wuya.x + wuya.width &&
                        characterX + 69 > wuya.x &&
                        characterY < wuya.y + wuya.height &&
                        characterY + 69 > wuya.y) {

                        lives--;
                        if (lives > 0) {
                            isInvincible = true;
                            invincibleTimer = INVINCIBLE_DURATION;
                            int newRespawnX, newRespawnY;
                            GetNearestRespawnPoint(characterX, characterY, facingRight, newRespawnX, newRespawnY, currentLevel);
                            characterX = newRespawnX;
                            characterY = newRespawnY;
                            facingRight = !facingRight;
                            isJumping = false;
                            jumpHeight = 0;
                            PlayCharacterVoice();
                        } else {
                            gameOver = true;
                            continue;
                        }
                    }
                }

                // 绘制夜晚的第一个乌鸦
                if (wuya.facingRight) {
                    putimage(wuya.x, wuya.y, &wuya.imgNightRightMask, SRCAND);
                    putimage(wuya.x, wuya.y, &wuya.imgNightRight, SRCPAINT);
                } else {
                    putimage(wuya.x, wuya.y, &wuya.imgNightLeftMask, SRCAND);
                    putimage(wuya.x, wuya.y, &wuya.imgNightLeft, SRCPAINT);
                }
            }

            // 第二个乌鸦的逻辑（与第一个相同）
            if (isDaytime) {
                // 白天：第二个乌鸦为中立状态，静止不动，作为障碍物平台
                putimage(wuya2.x, wuya2.y, &wuya2.imgDaytimeMask, SRCAND);
                putimage(wuya2.x, wuya2.y, &wuya2.imgDaytime, SRCPAINT);

                // 白天第二个乌鸦作为障碍物的碰撞检测（类似小凸起）
                bool horizontalOverlap2 = characterX + 69 > wuya2.x && characterX < wuya2.x + wuya2.width;
                bool verticalOverlap2 = characterY + 69 > wuya2.y && characterY < wuya2.y + wuya2.height;

                // 检查靠近自动上去的功能
                bool nearWuya2 = horizontalOverlap2 && characterY + 69 >= wuya2.y - 10 && characterY + 69 <= wuya2.y + 10;
                if (nearWuya2 && !isJumping) {
                    // 靠近时自动上去
                    characterY = wuya2.y - 69;
                    isJumping = false;
                    jumpHeight = 0;
                }

                // 正常的障碍物碰撞检测
                if (horizontalOverlap2 && verticalOverlap2) {
                    // 计算各个方向的重叠距离
                    int overlapLeft2 = characterX + 69 - wuya2.x;
                    int overlapRight2 = wuya2.x + wuya2.width - characterX;
                    int overlapTop2 = characterY + 69 - wuya2.y;
                    int overlapBottom2 = wuya2.y + wuya2.height - characterY;

                    // 找到最小的重叠距离，决定推出方向
                    int minOverlap2 = overlapLeft2;
                    if (overlapRight2 < minOverlap2) minOverlap2 = overlapRight2;
                    if (overlapTop2 < minOverlap2) minOverlap2 = overlapTop2;
                    if (overlapBottom2 < minOverlap2) minOverlap2 = overlapBottom2;

                    if (minOverlap2 == overlapLeft2 && overlapLeft2 < 20) {
                        // 从左侧推出
                        characterX = wuya2.x - 69;
                    } else if (minOverlap2 == overlapRight2 && overlapRight2 < 20) {
                        // 从右侧推出
                        characterX = wuya2.x + wuya2.width;
                    } else if (minOverlap2 == overlapTop2 && overlapTop2 < 20) {
                        // 从上方推出（站在乌鸦上）
                        characterY = wuya2.y - 69;
                        if (jumpHeight < 0) {
                            isJumping = false;
                            jumpHeight = 0;
                        }
                    } else if (minOverlap2 == overlapBottom2 && overlapBottom2 < 20) {
                        // 从下方推出（撞到乌鸦底部）
                        characterY = wuya2.y + wuya2.height;
                        if (jumpHeight > 0) {
                            jumpHeight = 0;
                        }
                    }
                }

                // 检查是否站在第二个乌鸦上（从上方接触）
                if (characterX + 60 > wuya2.x && characterX + 10 < wuya2.x + wuya2.width) {
                    if (characterY + 69 >= wuya2.y && characterY + 69 <= wuya2.y + 5 && characterY < wuya2.y) {
                        characterY = wuya2.y - 69;
                        if (jumpHeight < 0) {
                            isJumping = false;
                            jumpHeight = 0;
                        }
                    }
                }
            } else {
                // 夜晚：第二个乌鸦为敌对状态
                // 检查是否在同一层（根据第二个乌鸦的高度层）
                bool onSameLevel2 = (characterY + 69 >= 235 - 20 && characterY + 69 <= 235 + 20) &&
                                   (wuya2.y + wuya2.height >= 235 - 20 && wuya2.y + wuya2.height <= 235 + 20);

                // 检查是否能看到玩家（视野范围内且同一层）
                int distanceToPlayer2 = abs(characterX - wuya2.x);
                bool canSeePlayer2 = onSameLevel2 && distanceToPlayer2 <= 200; // 视野范围200像素

                if (canSeePlayer2) {
                    // 看到玩家时，朝玩家方向移动
                    if (characterX < wuya2.x) {
                        wuya2.facingRight = false;
                        wuya2.x -= wuya2.moveSpeed;
                        if (wuya2.x < wuya2.minX) {
                            wuya2.x = wuya2.minX;
                        }
                    } else if (characterX > wuya2.x + wuya2.width) {
                        wuya2.facingRight = true;
                        wuya2.x += wuya2.moveSpeed;
                        if (wuya2.x + wuya2.width > wuya2.maxX) {
                            wuya2.x = wuya2.maxX - wuya2.width;
                        }
                    }
                } else {
                    // 没看到玩家时，正常巡逻
                    if (wuya2.facingRight) {
                        wuya2.x += wuya2.moveSpeed;
                        if (wuya2.x + wuya2.width >= wuya2.maxX) {
                            wuya2.x = wuya2.maxX - wuya2.width;
                            wuya2.facingRight = false;
                        }
                    } else {
                        wuya2.x -= wuya2.moveSpeed;
                        if (wuya2.x <= wuya2.minX) {
                            wuya2.x = wuya2.minX;
                            wuya2.facingRight = true;
                        }
                    }
                }

                // 碰撞检测（只在同一层且非无敌状态）
                if (onSameLevel2 && !isInvincible) {
                    if (characterX < wuya2.x + wuya2.width &&
                        characterX + 69 > wuya2.x &&
                        characterY < wuya2.y + wuya2.height &&
                        characterY + 69 > wuya2.y) {

                        lives--;
                        if (lives > 0) {
                            isInvincible = true;
                            invincibleTimer = INVINCIBLE_DURATION;
                            int newRespawnX, newRespawnY;
                            GetNearestRespawnPoint(characterX, characterY, facingRight, newRespawnX, newRespawnY, currentLevel);
                            characterX = newRespawnX;
                            characterY = newRespawnY;
                            facingRight = !facingRight;
                            isJumping = false;
                            jumpHeight = 0;
                            PlayCharacterVoice();
                        } else {
                            gameOver = true;
                            continue;
                        }
                    }
                }

                // 绘制夜晚的第二个乌鸦
                if (wuya2.facingRight) {
                    putimage(wuya2.x, wuya2.y, &wuya2.imgNightRightMask, SRCAND);
                    putimage(wuya2.x, wuya2.y, &wuya2.imgNightRight, SRCPAINT);
                } else {
                    putimage(wuya2.x, wuya2.y, &wuya2.imgNightLeftMask, SRCAND);
                    putimage(wuya2.x, wuya2.y, &wuya2.imgNightLeft, SRCPAINT);
                }
            }
        }

        FlushBatchDraw();  // 刷新绘制内容

        // 检测键盘输入（A 和 D 控制左右移动）
        if (GetAsyncKeyState('A') & 0x8000) {
            characterX -= moveSpeed;
            facingRight = false; // 切换朝向为左
            // 防止角色超出窗口左侧
            if (characterX < 0) {
                characterX = 0;
            }
        }
        if (GetAsyncKeyState('D') & 0x8000) {
            characterX += moveSpeed;
            facingRight = true; // 切换朝向为右
            // 防止角色超出窗口右侧
            if (characterX > width - 69) { // 69 是角色宽度
                characterX = width - 69;
            }
        }

        // 退出条件：按下 ESC 键
        if (GetAsyncKeyState(VK_ESCAPE)) {
            break;
        }

        Sleep(10);  // 控制帧率
    }
    EndBatchDraw();  // 结束批量绘制
    // 在游戏结束时关闭所有音频
    mciSendString("close all", nullptr, 0, nullptr);
    // 关闭图形窗口
    closegraph();
    return 0;
}
