#define TESLA_INIT_IMPL 
#include <tesla.hpp>    
#include <vector>
#include <string>

struct XYcoord {
  s16 x;
  s16 y;
  XYcoord(u16 x, u16 y): x(x), y(y){}
};
class SnakeElement : public tsl::elm::Element {
public:
    static bool paused;
    
    SnakeElement(u16 x, u16 y, u16 w, u16 h, tsl::gfx::Color color) : Element(), _w(w), _h(h), _color(color), apple(200,200){
        snekBody = std::vector<XYcoord>();
        std::srand(std::time(0));
        setupSnek();
    }
    void setupSnek(){
        snekBody.clear();
        apple.x = 200;
        apple.y = 200;
        snekBody.push_back(apple);
        apple.x -= _w;
        snekBody.push_back(apple);
        apple.x -= _w;
        snekBody.push_back(apple);
        apple.x -= _w;
        snekBody.push_back(apple);
        pickAppleSpawn();
    }
    virtual void draw(tsl::gfx::Renderer* renderer) override{
        renderer->drawRect(0,75, tsl::cfg::FramebufferWidth, 2, _color);
        for(u16 i = 0; i < snekBody.size(); i++){
            renderer->drawRect(snekBody.at(i).x, snekBody.at(i).y, _w, _h, _color);
        }
        renderer->drawRect(apple.x, apple.y, _w, _h, _color);
        if(SnakeElement::paused)
            renderer->drawString("paused", false, 200, 50, 30, a(_color));
    }
    virtual void layout(u16 parentX, u16 parentY, u16 parentWidth, u16 parentHeight) override {
                    this->setBoundaries(parentX, parentY, parentWidth, parentHeight);
    }
    void pickAppleSpawn(){
        apple.x = std::rand() % 21 * _w;
        apple.y = 80 + std::rand() % 27 * _h;
        for(u16 i = 0; i < snekBody.size(); i++){
            if(apple.x == snekBody.at(i).x  &&  apple.y == snekBody.at(i).y)
                pickAppleSpawn();
        }
    }
    bool updateSnek(u16 dx, u16 dy){
        XYcoord targetLocation(snekBody.at(0).x + dx * _w, snekBody.at(0).y + dy * _h);
        XYcoord targetBottom(targetLocation.x + _w, targetLocation.y + _h + dy);
        
        for(u16 i = 0; i < snekBody.size(); i++){
            if(targetLocation.x == snekBody.at(i).x  &&  targetLocation.y == snekBody.at(i).y)
                return false;
        }
        //80 and 40 are top and bottom offsets
        if(targetLocation.x < 0 || targetLocation.y < 80 || targetBottom.x > tsl::cfg::FramebufferWidth || targetBottom.y > tsl::cfg::FramebufferHeight - 40 - _h)
            return false;
        if(targetLocation.x == apple.x  &&  targetLocation.y == apple.y){
            snekBody.push_back(targetLocation);
            pickAppleSpawn();
        }
        for(u16 i = snekBody.size() - 1; i > 0; i--){
            snekBody.at(i) = snekBody.at(i - 1);
        }
        snekBody.at(0) = targetLocation;
        return true;
    }
    u16 getLength(){
        return snekBody.size();
    }
private:
    std::vector<XYcoord> snekBody;
    u16 _w;
    u16 _h;
    tsl::gfx::Color _color;
    XYcoord apple;
};
bool SnakeElement::paused;


class SnekGui : public tsl::Gui {
public:
    SnekGui() : direction(1,0){ }
    ~SnekGui(){
    }
    virtual tsl::elm::Element* createUI() override {
        auto rootFrame = new tsl::elm::OverlayFrame("Snek", "v0.0.1");
        snake = new SnakeElement(0,0,20,20, tsl::gfx::Renderer::a(0xFFFF));
        timeSinceLastFrame = std::chrono::system_clock::now();
        rootFrame->setContent(snake);
        return rootFrame;
    }

    virtual void update() override {
        auto elapsed = std::chrono::system_clock::now() - timeSinceLastFrame;
         if(elapsed >= 1000ms / ((snake->getLength() * 0.6) / 2) && !SnakeElement::paused){
            if(!snake->updateSnek(direction.x, direction.y)){
                snake->setupSnek();
                direction.x = 1;
                direction.y = 0;
            }
            
            timeSinceLastFrame = std::chrono::system_clock::now();
         }
    }

    virtual bool handleInput(u64 keysDown, u64 keysHeld, touchPosition touchInput, JoystickPosition leftJoyStick, JoystickPosition rightJoyStick) override {
                if (keysDown & KEY_UP && direction.y != 1){
                    direction.y = -1;
                    direction.x = 0;
                }
                else if (keysDown & KEY_DOWN && direction.y != -1){
                    direction.y = 1;
                    direction.x = 0;
                }
                else if (keysDown & KEY_LEFT && direction.x != 1){
                    direction.x = -1;
                    direction.y = 0;
                }
                else if (keysDown & KEY_RIGHT && direction.x != -1){
                    direction.x = 1;
                    direction.y = 0;
                }
                else if (keysDown & KEY_PLUS){
                    SnakeElement::paused = !SnakeElement::paused;
                }
                else if (keysDown & KEY_A)
                    SnakeElement::paused = !SnakeElement::paused;
        return false;  
    }
private:
    SnakeElement * snake;
    XYcoord direction;
    std::chrono::time_point<std::chrono::system_clock> timeSinceLastFrame;
};


class SnekOverlay : public tsl::Overlay {
public:
    virtual void initServices() override {} 
    virtual void exitServices() override {}  

    virtual void onShow() override {}   
    virtual void onHide() override {SnakeElement::paused = true;}  

    virtual std::unique_ptr<tsl::Gui> loadInitialGui() override {
        return initially<SnekGui>(); 
    }
    
};

int main(int argc, char **argv) {
    return tsl::loop<SnekOverlay>(argc, argv);
}
