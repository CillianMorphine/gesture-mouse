#pragma once

/**
 * @brief Низькорівневий рівень симуляції вводу.
 *        MouseController використовує цей клас для відправки подій ОС.
 */
class InputSimulator {
public:
    static void moveMouse(int x, int y);
    static void mouseButtonDown(int button);   // 1=left, 2=middle, 3=right
    static void mouseButtonUp(int button);
    static void mouseScroll(int delta);        // >0 вгору, <0 вниз
    static void keyDown(int keyCode);
    static void keyUp(int keyCode);
};
