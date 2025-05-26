#include <graphics.h>
#include <iostream>
#include <vector>
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
void GetNearestRespawnPoint(int currentX, int currentY, bool facingRight, int& respawnX, int& respawnY) {
    // 定义重生点列表
    struct RespawnPoint {
        int x, y;
    };
    
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
    bool gameOver = false;      // 添加游戏结束标志
    bool shouldRestart = false; // 添加重新开始标志
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

    // 加载白天和黑夜背景图片
    IMAGE daytime, evening;
    loadimage(&daytime, "D:\\Cyber-Space\\c++\\Gemshin-games\\pictures\\daytime.png", width, height);
    loadimage(&evening, "D:\\Cyber-Space\\c++\\Gemshin-games\\pictures\\evening.png", width, height);

    // 加载角色图片和对应的掩码图片
    IMAGE characterLeft, characterLeftMask;
    IMAGE characterRight, characterRightMask;
    IMAGE niaoju, niaoujuMask;
    IMAGE tofu, tofuMask;
    
    // 加载原始图像
    loadimage(&characterLeft, "D:\\Cyber-Space\\c++\\Gemshin-games\\pictures\\aqiao-l.png", 69, 69);
    loadimage(&characterRight, "D:\\Cyber-Space\\c++\\Gemshin-games\\pictures\\aqiao-r.png", 69, 69);
    loadimage(&niaoju, "D:\\Cyber-Space\\c++\\Gemshin-games\\pictures\\niaoju.png", 50, 50);
    loadimage(&tofu, "D:\\Cyber-Space\\c++\\Gemshin-games\\pictures\\tofu.png", 50, 50);
    
    // 加载掩码图像
    loadimage(&characterLeftMask, "D:\\Cyber-Space\\c++\\Gemshin-games\\pictures\\aqiao-l-mask.png", 69, 69);
    loadimage(&characterRightMask, "D:\\Cyber-Space\\c++\\Gemshin-games\\pictures\\aqiao-r-mask.png", 69, 69);
    loadimage(&niaoujuMask, "D:\\Cyber-Space\\c++\\Gemshin-games\\pictures\\niaoju-mask.png", 50, 50);
    loadimage(&tofuMask, "D:\\Cyber-Space\\c++\\Gemshin-games\\pictures\\tofu-mask.png", 50, 50);

    // 加载蘑菇怪图片
    IMAGE moguLeft, moguLeftMask, moguRight, moguRightMask;
    loadimage(&moguLeft, "D:\\Cyber-Space\\c++\\Gemshin-games\\pictures\\mogu-l.png", 50, 50); // 假设蘑菇怪大小为50x50
    loadimage(&moguLeftMask, "D:\\Cyber-Space\\c++\\Gemshin-games\\pictures\\mogu-l-mask.png", 50, 50);
    loadimage(&moguRight, "D:\\Cyber-Space\\c++\\Gemshin-games\\pictures\\mogu-r.png", 50, 50);
    loadimage(&moguRightMask, "D:\\Cyber-Space\\c++\\Gemshin-games\\pictures\\mogu-r-mask.png", 50, 50);

    // 检查图像是否成功加载
    if (characterRight.getwidth() == 0 || characterRightMask.getwidth() == 0 ||
        characterLeft.getwidth() == 0 || characterLeftMask.getwidth() == 0 ||
        niaoju.getwidth() == 0 || niaoujuMask.getwidth() == 0 ||
        tofu.getwidth() == 0 || tofuMask.getwidth() == 0 ||
        moguLeft.getwidth() == 0 || moguLeftMask.getwidth() == 0 || // 检查蘑菇怪图片
        moguRight.getwidth() == 0 || moguRightMask.getwidth() == 0) {
        std::cout << "图像加载失败！请检查路径。" << std::endl;
        closegraph();
        return 1;
    }

    // 初始背景为白天
    bool isDaytime = true;
    bool hasEatenTofu = false;  // 添加变量跟踪豆腐是否被吃掉
    bool showFKey = false;      // 是否显示F键提示

    // 角色初始位置和状态
    int characterX = 50;
    int characterY = 340;
    int moveSpeed = 5;
    int jumpSpeed = 12;  // 跳跃初始速度
    int gravity = 1;     // 重力
    int initialY = 342; // 角色初始y坐标作为地面高度
    bool facingRight = true;
    bool isJumping = false;
    int jumpHeight = 0;
    bool onPlatform = false;

    // 初始化蘑菇怪
    Mogu mogu = {
        630, 399 - 50, // 初始位置在 (485, 399) 的上方，因为蘑菇怪有高度
        50, 50,       // 宽高
        2,            // 移动速度
        true,         // 初始朝向右
        630, 630 + 405, // 移动范围 (例如，在485右侧100像素内移动)
        moguLeft, moguLeftMask,
        moguRight, moguRightMask
    };
    
    Mogu mogu2 = {
        1025, 399 - 50, // 初始位置在 (1025, 399) 的上方
        50, 50,       // 宽高
        2,            // 移动速度
        false,        // 初始朝向左
        1025 - 405, 1025, // 移动范围
        moguLeft, moguLeftMask,
        moguRight, moguRightMask
    };

    // 在加载图片后，游戏开始前定义障碍物和死亡区域
    std::vector<Obstacle> obstacles = {
        {174, 300, 47, 94, RGB(255, 192, 203)},  // 左边高墙
        {820, 320, 190, 30, RGB(255, 192, 203)}    // 右边二楼台阶
    };

    // 定义死亡区域（黑夜模式下的坑）
    std::vector<DeathZone> deathZones = {
        {316, 424, 50, 25},   // 第一个坑
        {485, 424, 145, 25}    // 第二个坑
    };

    // 定义透明区域（黑夜模式下可以穿过的区域）
    std::vector<TransparentZone> transparentZones = {
        {316, 399, 50, 25},   // 第一个坑上方的透明区域
        {485, 399, 145, 25}    // 第二个坑上方的透明区域
    };

    // 定义重生点
    int respawnX = 50;
    int respawnY = 340;

    // 绘制初始背景和角色
    putimage(0, 0, &daytime);
    
    // 使用SRCAND和SRCPAINT组合实现透明效果
    putimage(characterX, characterY, &characterRightMask, SRCAND);
    putimage(characterX, characterY, &characterRight, SRCPAINT);

    // 消息处理循环
    ExMessage msg;
    while (true) {
        if (gameOver) {
            mciSendString("close all", nullptr, 0, nullptr);
            cleardevice();
            putimage(0, 0, &evening);
            settextcolor(WHITE);
            LOGFONT font;
            gettextstyle(&font);
            font.lfHeight = 40;
            _tcscpy(font.lfFaceName, _T("微软雅黑"));
            font.lfQuality = ANTIALIASED_QUALITY;
            settextstyle(&font);
            setbkmode(TRANSPARENT);
            RECT textRect = {width/2 - 100, height/2 - 100, width/2 + 100, height/2 - 60};
            drawtext(_T("游戏结束"), &textRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            
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

        // 处理所有绘制操作
        BeginBatchDraw();

        // 清空画面并绘制背景
        cleardevice();
        if (isDaytime) {
            putimage(0, 0, &daytime);
        } else {
            putimage(0, 0, &evening);
        }

        // 更新无敌时间
        if (isInvincible) {
            invincibleTimer--;
            if (invincibleTimer <= 0) {
                isInvincible = false;
            }
        }

        // 获取鼠标消息（用于切换背景）
        if (peekmessage(&msg, EX_MOUSE)) {
            // 检测鼠标右键点击，切换背景
            if (msg.message == WM_RBUTTONDOWN) {
                isDaytime = !isDaytime;
                cleardevice(); // 清空画面
                if (isDaytime) {
                    putimage(0, 0, &daytime); // 显示白天背景
                    // 检查角色是否在障碍物内部
                    for (const auto& obstacle : obstacles) {
                        // 检查是否与障碍物发生碰撞
                        if (characterX + 60 > obstacle.x && characterX + 10 < obstacle.x + obstacle.width) {
                            if (characterY + 69 > obstacle.y && characterY < obstacle.y + obstacle.height) {
                                // 计算到上方和下方的距离
                                int distToTop = characterY + 69 - obstacle.y;
                                int distToBottom = (obstacle.y + obstacle.height) - characterY;
                                
                                // 选择最近的安全位置
                                if (distToTop < distToBottom) {
                                    // 移动到障碍物上方
                                    characterY = obstacle.y - 69;
                                    isJumping = false;
                                    jumpHeight = 0;
                                    onPlatform = true;
                                } else {
                                    // 移动到障碍物下方
                                    characterY = obstacle.y + obstacle.height;
                                    isJumping = true;
                                    jumpHeight = 0;
                                    onPlatform = false;
                                }
                                break;
                            }
                        }
                    }
                } else {
                    putimage(0, 0, &evening); // 显示黑夜背景
                    // 切换到黑夜时取消所有平台碰撞状态
                    onPlatform = false;
                }
            }
        }

        // 处理跳跃
        if (GetAsyncKeyState(VK_SPACE) & 0x8000 && !isJumping) {
            isJumping = true;
            jumpHeight = jumpSpeed;
        }

        // 更新角色位置
        if (isJumping) {
            int nextY = characterY - jumpHeight;  // 计算下一帧的位置

            // 只在白天模式下进行碰撞检测
            if (isDaytime) {
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
                // 水平方向碰撞检测（只检测墙壁，不检测平台）
                if (obstacle.height > 30) {  // 假设高度大于30的是墙壁
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

                // 检查是否站在平台上（只检查从上方接触）
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

            // 更新平台状态
            onPlatform = platformCollision;

            // 添加地面碰撞检测
            if (!onPlatform && characterY >= initialY) {
                characterY = initialY;
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
                // 不在透明区域时，检查是否在地面
                if (characterY >= initialY) {
                    characterY = initialY;
                    isJumping = false;
                    jumpHeight = 0;
                    onPlatform = true;
                } else if (!isJumping) {
                    isJumping = true;
                    jumpHeight = 0;
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
                            GetNearestRespawnPoint(characterX, characterY, facingRight, newRespawnX, newRespawnY);
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
                            break;  // 跳过当前帧的剩余处理
                        } else {
                            gameOver = true;
                            break;
                        }
                    }
                }

                // 蘑菇怪碰撞检测
                if (!gameOver) {
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
                            GetNearestRespawnPoint(characterX, characterY, facingRight, newRespawnX, newRespawnY);
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
                            break;
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
                            GetNearestRespawnPoint(characterX, characterY, facingRight, newRespawnX, newRespawnY);
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
                            break;
                        }
                    }
                }
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
                setfillcolor(obstacle.color);
                solidrectangle(obstacle.x, obstacle.y, obstacle.x + obstacle.width, obstacle.y + obstacle.height);
            }
            
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
        
        // 在绘制角色之前，根据当前模式绘制鸟居或豆腐
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
                    // 进入下一关
                    MessageBoxW(GetHWnd(), L"恭喜通关！", L"提示", MB_OK);
                    return 0;  // 或者这里可以加载下一关的逻辑
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
                    // 可以在这里添加音效或其他效果
                }
            }
            // 在黑夜模式下强制关闭F键显示
            showFKey = false;
        }

        // 如果需要显示F键提示（只在白天模式下）
        if (showFKey && isDaytime) {
            settextcolor(RGB(255, 255, 255));  // 白色文字
            settextstyle(20, 0, "宋体");       // 缩小字体大小
            setbkmode(TRANSPARENT);            // 设置文字背景透明
            outtextxy(820 + 140 + 15, 320 - 70, "F");  // 调整位置到鸟居正上方
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

        // 更新和绘制蘑菇怪 (只在黑夜)
        if (!isDaytime) {
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

        // 避免 CPU 占用过高
        Sleep(10);
    }

    EndBatchDraw();  // 结束批量绘制
    // 在游戏结束时关闭所有音频
    mciSendString("close all", nullptr, 0, nullptr);
    // 关闭图形窗口
    closegraph();
    return 0;
}
