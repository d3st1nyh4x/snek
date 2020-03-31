#define TESLA_INIT_IMPL 
#include <tesla.hpp>    
#include <vector>
#include <string>
#include <sstream>
#include <stdio.h>
#include <iostream>
#include <ios>

struct XYcoord {
  s16 x;
  s16 y;
  XYcoord(s16 x, s16 y): x(x), y(y){}
};

std::ostream& operator<<(std::ostream& stream, const XYcoord& pair) {
	return stream << pair.x << ',' << pair.y << std::endl;
};

std::istream& operator>>(std::istream& stream, XYcoord& pair) {
	char c;
	if(stream >> pair.x >> c >> pair.y && c != ',' ){
		stream.setstate(std::ios::failbit);
	}
	return stream;
};


class SnakeElement : public tsl::elm::Element {
public:
    static bool paused;
    
    SnakeElement(u16 w, u16 h, tsl::gfx::Color color, std::vector<XYcoord> * v, XYcoord * a) : Element(), _w(w), _h(h), _color(color){
        snekBody = v;
        apple = a;
    }
    ~SnakeElement(){
    }
    virtual void draw(tsl::gfx::Renderer* renderer) override{
        score.str(std::string());
        score << "High: " << getHighScore() << std::endl << "Score: " << getScore();
        renderer->drawRect(0,75, tsl::cfg::FramebufferWidth, 2, _color);
        for(u16 i = 0; i < snekBody->size(); i++){
            renderer->drawRect(snekBody->at(i).x, snekBody->at(i).y, _w, _h, _color);
        }
        renderer->drawRect(apple->x, apple->y, _w, _h, _color);
        if(SnakeElement::paused){
            renderer->drawString("paused", false, 200, 50, 30, a(_color));
        }
        else{
            renderer->drawString(score.str().c_str(), false, 120, 29, 30, a(_color));
        }
    }
    virtual void layout(u16 parentX, u16 parentY, u16 parentWidth, u16 parentHeight) override {
                    this->setBoundaries(parentX, parentY, parentWidth, parentHeight);
    }
    u16 getScore(){
        return snekBody->size() ? (snekBody->size() - 4) * 100 : 0;
    }
    void setHighScore(u16 s){
        highScore = s;
    }
    u16 getHighScore(){
        return highScore;
    }
private:
    std::vector<XYcoord> * snekBody;
    XYcoord * apple;
    u16 _w;
    u16 _h;
    tsl::gfx::Color _color;
    std::ostringstream score;
    u16 highScore;
};
bool SnakeElement::paused;


class SnekGui : public tsl::Gui {
public:
    SnekGui(std::string s, std::vector<XYcoord> * v) : direction(1,0), lastDir(-1,0), apple(200,200){
        saved = s;
        std::srand(std::time(0));
        highScore = 0;
        _w = 20;
        _h = _w;
        snekBody = v;
    }
    ~SnekGui(){}
    virtual tsl::elm::Element* createUI() override {
        auto rootFrame = new tsl::elm::OverlayFrame("Snek", "v0.8.0");
        snake = new SnakeElement(_w,_h, tsl::gfx::Renderer::a(0xFFFF), snekBody, &apple);
        timeSinceLastFrame = std::chrono::system_clock::now();
        rootFrame->setContent(snake);
        std::istringstream save;
        save.str(saved);
        save >> highScore;
        save >> apple;
        save >> direction;
        save >> lastDir;
        for(XYcoord x(0,0); save >> x; snekBody->push_back(x));
        snake->setHighScore(highScore);
        finalizeSnek();
        
        return rootFrame;
    }
    
    virtual void update() override {
        auto elapsed = std::chrono::system_clock::now() - timeSinceLastFrame;
         if(elapsed >= 1000ms / ((snekBody->size() * 0.6) / 2) && !SnakeElement::paused){
            lastDir.x = direction.x;
            lastDir.y = direction.y;
            if(!updateSnek(direction.x, direction.y)){
                setupSnek();
            }
            timeSinceLastFrame = std::chrono::system_clock::now();
         }
    }

    virtual bool handleInput(u64 keysDown, u64 keysHeld, touchPosition touchInput, JoystickPosition leftJoyStick, JoystickPosition rightJoyStick) override {
                if (keysDown & KEY_UP && lastDir.y != 1){
                    direction.y = -1;
                    direction.x = 0;
                }
                else if (keysDown & KEY_DOWN && lastDir.y != -1){
                    direction.y = 1;
                    direction.x = 0;
                }
                else if (keysDown & KEY_LEFT && lastDir.x != 1){
                    direction.x = -1;
                    direction.y = 0;
                }
                else if (keysDown & KEY_RIGHT && lastDir.x != -1){
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
    
    std::string getSaveFile(){
        std::ostringstream s;
        s << highScore << std::endl;
        s << apple << direction << lastDir;
        for(XYcoord c : *snekBody){
            s << c;
        }
        return s.str();
    }
    
    void finalizeSnek(){
        if(snekBody->size() == 0){
            setupSnek();
        }
        else{
            SnakeElement::paused = true;
        }
    }
    
    bool updateSnek(u16 dx, u16 dy){
        XYcoord targetLocation(snekBody->at(0).x + dx * _w, snekBody->at(0).y + dy * _h);
        XYcoord targetBottom(targetLocation.x + _w, targetLocation.y + _h + dy);
        
        for(u16 i = 0; i < snekBody->size(); i++){
            if(targetLocation.x == snekBody->at(i).x  &&  targetLocation.y == snekBody->at(i).y)
                return false;
        }
        //80 and 40 are top and bottom offsets
        if(targetLocation.x < 0 || targetLocation.y < 80 || targetBottom.x > tsl::cfg::FramebufferWidth || targetBottom.y > tsl::cfg::FramebufferHeight - 40 - _h)
            return false;
        if(targetLocation.x == apple.x  &&  targetLocation.y == apple.y){
            snekBody->push_back(targetLocation);
            pickAppleSpawn();
        }
        for(u16 i = snekBody->size() - 1; i > 0; i--){
            snekBody->at(i) = snekBody->at(i - 1);
        }
        snekBody->at(0) = targetLocation;
        return true;
    }
    
    void pickAppleSpawn(){
        apple.x = std::rand() % 21 * _w;
        apple.y = 80 + std::rand() % 27 * _h;
        for(u16 i = 0; i < snekBody->size(); i++){
            if(apple.x == snekBody->at(i).x  &&  apple.y == snekBody->at(i).y)
                pickAppleSpawn();
        }
    }
    void setupSnek(){
        if(snake->getScore() > snake->getHighScore()){
            highScore = snake->getScore();
            snake->setHighScore(highScore);
        }
        snekBody->clear();
        apple.x = 200;
        apple.y = 200;
        snekBody->push_back(apple);
        apple.x -= _w;
        snekBody->push_back(apple);
        apple.x -= _w;
        snekBody->push_back(apple);
        apple.x -= _w;
        snekBody->push_back(apple);
        pickAppleSpawn();
        direction.x = 1;
        direction.y = 0;
        lastDir.x = 1;
        lastDir.y = 0;
    }
    
private:
    SnakeElement * snake;
    XYcoord direction;
    XYcoord lastDir;
    std::chrono::time_point<std::chrono::system_clock> timeSinceLastFrame;
    std::string saved;
    u16 highScore;
    std::vector<XYcoord> * snekBody;
    XYcoord apple;
    u16 _w;
    u16 _h;
};


class SnekOverlay : public tsl::Overlay {
public:
    void getSave(){
        FsFileSystem fsSdmc;
        if (R_FAILED(fsOpenSdCardFileSystem(&fsSdmc)))
            return;
        tsl::hlp::ScopeGuard fsGuard([&] { fsFsClose(&fsSdmc); });
        FsFile saveFile;
        if (R_FAILED(fsFsOpenFile(&fsSdmc, "/switch/snek", FsOpenMode_Read, &saveFile)))
            return;
        tsl::hlp::ScopeGuard fileGuard([&] { fsFileClose(&saveFile); });
        s64 saveFileSize;
        if (R_FAILED(fsFileGetSize(&saveFile, &saveFileSize)))
            return;
        std::string saveFileData(saveFileSize, '\0');
        u64 readSize;
        Result rc = fsFileRead(&saveFile, 0, saveFileData.data(), saveFileSize, FsReadOption_None, &readSize);
        if (R_FAILED(rc) || readSize != static_cast<u64>(saveFileSize))
            return;
        savedSnek = saveFileData;
    }
    void setSave(std::string s){
        FsFileSystem fsSdmc;
        if (R_FAILED(fsOpenSdCardFileSystem(&fsSdmc)))
            return;
        tsl::hlp::ScopeGuard fsGuard([&] { fsFsClose(&fsSdmc); });
        FsFile saveFile;
        fsFsDeleteFile(&fsSdmc, "/switch/snek");
        if (R_FAILED(fsFsCreateFile(&fsSdmc, "/switch/snek", s.length(), 0)))
                return;
        if (R_FAILED(fsFsOpenFile(&fsSdmc, "/switch/snek", FsOpenMode_Write, &saveFile)))
                return;
        tsl::hlp::ScopeGuard fileGuard([&] { fsFileClose(&saveFile); });
        
        fsFileWrite(&saveFile, 0, s.c_str(), s.length(), FsWriteOption_Flush);
    }
    
    virtual void initServices() override {
    } 
    
    virtual void exitServices() override {
        setSave(gaemGui->getSaveFile());
    }  

    virtual void onShow() override {}   
    virtual void onHide() override {SnakeElement::paused = true;}  

    virtual std::unique_ptr<tsl::Gui> loadInitialGui() override {
        getSave();
        auto r = initially<SnekGui>(savedSnek, &snekBody); 
        savedSnek.clear();
        gaemGui = (SnekGui*) r.get();
        return r;
    }
private:
    std::string savedSnek;
    SnekGui* gaemGui;
    std::vector<XYcoord> snekBody;
};

int main(int argc, char **argv) {
    return tsl::loop<SnekOverlay>(argc, argv);
}
