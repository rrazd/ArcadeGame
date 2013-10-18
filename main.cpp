#include <cstdlib>
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <list>
#include <sys/time.h>
#include <cmath>
#include <sstream>
//main Xlib headers
#include <X11/Xlib.h>
#include <X11/Xutil.h>

using namespace std;

#define BufferSize 10
#define Border 10
#define EndOfLeftScreenWalk 783
#define EndOfRightScreenWalk 11
int FPS;
bool isStartScreen = true; //when program is first run we have startScreen appear
bool isPaused = true;
double sunSpeed; //needed beacuse speed of sun should be independent of FPS
double marioRunningSpeed; //needed beacuse running speed should be independent of FPS
double marioJumpingSpeed; //needed beacuse jumping speed should be independent of FPS
double delta = 0;
int score;
bool isJumping;
bool isRunning;
string scoreString, scoreMessage, pointMessage;
bool startSuccessful = false;

//STRUCTS

struct XInfo {
    Display* display;
    Window	 window;
    int screen;
    GC gc[3];
    int	width;		// size of window
	int	height;
    Pixmap pixmap;  // double buffer
    XColor xColor;
    Colormap colorMap;
};




//CLASSES

//An abstract class representing displayable things.
class Displayable {
public:
    virtual void paint(XInfo &xInfo) = 0;
};

list<Displayable *> dList; //list of Displayables (needed to be declared at the top because it is used by some of the class methods)


class Money : public Displayable{
public:
    virtual void paint(XInfo &xInfo){
        XDrawArc(xInfo.display, xInfo.pixmap, xInfo.gc[0], x, y, width, height, 0, 360*64);
    }
    
    //constructor
    Money(int x, int y) : x(x), y(y), height(35), width(40) {}
private:
    int x;
    int y;
    int height;
    int width;
};
Money *money4 = new Money(30, 365);
Money *money3 = new Money(730, 415);
Money *money2 = new Money(550, 516);
Money *money = new Money(150, 516);



class Spike : public Displayable{
public:
    virtual void paint(XInfo &xInfo){
        XPoint points[] = {
            {x+25, y+80},
            {x+50, y+50},
            {x+50, y+50},
            {x+75, y+80}
        };
        int npoints = sizeof(points)/sizeof(XPoint);
        
        XDrawLines(xInfo.display, xInfo.pixmap, xInfo.gc[0], points, npoints, CoordModeOrigin);
    }
    Spike(int x, int y): x(x), y(y){}
private:
    int x, y;
    
};
Spike *spike = new Spike(300, 420);




class Sun : public Displayable{
public:
    virtual void paint(XInfo &xInfo){
        
        XFillArc(xInfo.display, xInfo.pixmap, xInfo.gc[0], x, 104, 55, 50, 0, 360*64);
        //anticlockwise starting from 12 o'clock position
        XDrawLine(xInfo.display, xInfo.pixmap, xInfo.gc[0], x + 25, 84, x+50, 135);
        XDrawLine(xInfo.display, xInfo.pixmap, xInfo.gc[0], x + 25, 84, x, 135);
        
        XDrawLine(xInfo.display, xInfo.pixmap, xInfo.gc[0], x + 20, 124, x+80, 75);
        XDrawLine(xInfo.display, xInfo.pixmap, xInfo.gc[0], x + 50, 120, x+80, 75);
        
        XDrawLine(xInfo.display, xInfo.pixmap, xInfo.gc[0], x - 25, 134, x+40, 105);
        XDrawLine(xInfo.display, xInfo.pixmap, xInfo.gc[0], x - 25, 134, x+40, 145);
        
        XDrawLine(xInfo.display, xInfo.pixmap, xInfo.gc[0], x - 25, 75, x+35, 124);
        XDrawLine(xInfo.display, xInfo.pixmap, xInfo.gc[0], x - 25, 75, x+5, 125);
        
        XDrawLine(xInfo.display, xInfo.pixmap, xInfo.gc[0], x + 25, 175, x + 50, 125);
        XDrawLine(xInfo.display, xInfo.pixmap, xInfo.gc[0], x + 25, 175, x, 125);
        
        XDrawLine(xInfo.display, xInfo.pixmap, xInfo.gc[0], x - 30, 175, x+15, 128);
        XDrawLine(xInfo.display, xInfo.pixmap, xInfo.gc[0], x - 30, 175, x+20, 147);
        
        XDrawLine(xInfo.display, xInfo.pixmap, xInfo.gc[0], x + 80, 130, x + 10, 105);
        XDrawLine(xInfo.display, xInfo.pixmap, xInfo.gc[0], x + 80, 130, x + 10, 145);
        
        XDrawLine(xInfo.display, xInfo.pixmap, xInfo.gc[0], x + 10, 127, x+85, 175);
        XDrawLine(xInfo.display, xInfo.pixmap, xInfo.gc[0], x + 30, 110, x+85, 175);
    }
    
    void move(XInfo &xinfo) {
        x = x + distance;
    }
    
    void setDistance(){
        distance = sunSpeed * delta;
        
        if (isPaused) {
            distance = 0;
        }
        else if (goingRight) {
            if(x>722){
                distance = -distance;
                goingRight = false;
            }
        }
        else if (!goingRight) {
            distance = -distance;
            if (x<22) {
                goingRight = true;
            }
        }
    }
    
    //constructor
    Sun(){
        x = 22.0;
        goingRight = true; //initially sun should be going right
    }
    
private:
    bool goingRight;
    double distance;
    double x;
};
Sun *sun = new Sun();




class Text : public Displayable {
public:
    virtual void paint(XInfo &xInfo) {
        XDrawImageString( xInfo.display, xInfo.pixmap, xInfo.gc[0],
                         (int)this->x, (int)this->y, this->s.c_str(), (int)this->s.length() );
    }
    
    // constructor
    Text(int x, int y, string s):x(x), y(y), s(s)  {}
    
private:
    //XPoint p; // a 2D point
    int x;
    int y;
    string s; // string to show
};


class Block : public Displayable {
public:
    virtual void paint(XInfo &xInfo){
        // draw into the buffer
        XFillRectangle(xInfo.display, xInfo.pixmap, xInfo.gc[0], x, y, width, height);
    }
    
    double getCollisionFromRight_x1(){
        return collisionFromRight_x1;
    }
    
    double getCollisionFromRight_x2(){
        return collisionFromRight_x2;
    }
    
    double getCollisionFromLeft_x1(){
        return collisionFromLeft_x1;
    }
    
    double getCollisionFromLeft_x2(){
        return collisionFromLeft_x2;
    }
    
    
    double getCollisionFromTop(){
        return collisionFromTop;
    }
    
    double getStopMovingDown(){
        return stopMovingDown;
    }
    
    double getX(){
        return x;
    }
    
    double getY(){
        return y;
    }
    
    
    // constructor
    Block(int x_coord, int y_coord, int w, int h){
        
        x = x_coord;
        y = y_coord;
        width = w;
        height = h;
        collisionFromLeft_x1 = x + 112;
        collisionFromLeft_x2 = x;
        collisionFromRight_x1 = x - 18;
        collisionFromRight_x2 = x + 40;
        collisionFromTop = y + 33;
        stopMovingDown = y - 27;
    }
private:
    int x;
    int y;
    int width;
    int height;
    double collisionFromLeft_x1;
    double collisionFromLeft_x2;
    double collisionFromRight_x1;
    double collisionFromRight_x2;
    double collisionFromTop;
    double stopMovingDown;
};
Block *blockA = new Block(  0, 550, 100, 50);
Block *blockL = new Block(  0, 400, 100, 50);
Block *blockB = new Block(100, 550, 100, 50);
Block *blockC = new Block(200, 550, 100, 50);
Block *blockD = new Block(300, 550, 100, 50);
Block *blockE = new Block(300, 500, 100, 50);
Block *blockF = new Block(400, 550, 100, 50);
Block *blockG = new Block(500, 550, 100, 50);
Block *blockH = new Block(600, 550, 100, 50);
Block *blockI = new Block(700, 550, 100, 50);
Block *blockJ = new Block(700, 500, 100, 50);
Block *blockK = new Block(700, 450, 100, 50);
list<Block *> collisionList;


class Scale : public Displayable {
public:
    virtual void paint(XInfo &xInfo){
        
        XFillArc(xInfo.display,xInfo.pixmap,xInfo.gc[2],1,27,30,30,40*64,90*64);
        XFillArc(xInfo.display,xInfo.pixmap,xInfo.gc[1],2,17,30,30,60*64,60*64);
        XDrawRectangle(xInfo.display, xInfo.pixmap, xInfo.gc[0], 29, 30, 100, 15);
		XFillRectangle(xInfo.display, xInfo.pixmap, xInfo.gc[0], 29, 30, health, 15);
        
        XDrawArc(xInfo.display, xInfo.pixmap, xInfo.gc[0], 10, 50, 15, 15, 0, 360*64);
        XDrawRectangle(xInfo.display, xInfo.pixmap, xInfo.gc[0], 29, 50, 250, 15);
		XFillRectangle(xInfo.display, xInfo.pixmap, xInfo.gc[0], 29, 50, points, 15);
    }
    
    int getHealth(){
        return health;
    }
    
    void setHealth(int healthValue){
        health = healthValue;
    }
    
    int getPoints(){
        return points;
    }
    
    void setPoints(int pointValue){
        points = pointValue;
    }

    
    
    Scale(){
        health = 50;
        points = 50;
    }
private:
    int health;
    int points;
};
Scale* scale = new Scale();


class Heart : public Displayable {
public:
    virtual void paint(XInfo &xInfo){
        //draw red heart
 
        XFillArc(xInfo.display,xInfo.pixmap,xInfo.gc[2],x,y,50,50,40*64,90*64);
        XFillArc(xInfo.display,xInfo.pixmap,xInfo.gc[1],x2,y2,50,50,60*64,60*64);
        
    }
    Heart(int x_coord, int y_coord){
        x = x_coord;
        y = y_coord;
        x2 = x + 1;
        y2 = y - 19;
    }
    
private:
    int x;
    int y;
    int x2;
    int y2;
};
Heart *heart = new Heart(200, 400);



class Mario : public Displayable {
public:
    virtual void paint(XInfo &xInfo){
        XFillRectangle(xInfo.display, xInfo.pixmap, xInfo.gc[0], body_x, body_y, 27, 50);
        XFillRectangle(xInfo.display, xInfo.pixmap, xInfo.gc[0], leftLeg_x, leftLeg_y, 10, 20);
        XFillRectangle(xInfo.display, xInfo.pixmap, xInfo.gc[0], rightLeg_x, rightLeg_y, 10, 20);
        XFillArc(xInfo.display, xInfo.pixmap, xInfo.gc[0], head_x, head_y, 35, 30, 0, 360*64);
        if(facingRight){
            XFillArc(xInfo.display, xInfo.pixmap, xInfo.gc[1], eyeLookRight_x, eyeLookRight_y, 5, 5, 0, 360*64);
            XFillArc(xInfo.display, xInfo.pixmap, xInfo.gc[1], mouthLookRight_x, mouthLookRight_y, 11, 11, 0, 360*64);
        }
        else{
            XFillArc(xInfo.display, xInfo.pixmap, xInfo.gc[1], eyeLookLeft_x, eyeLookLeft_y, 5, 5, 0, 360*64);
            XFillArc(xInfo.display, xInfo.pixmap, xInfo.gc[1], mouthLookLeft_x, mouthLookLeft_y, 11, 11, 0, 360*64);
        }
        
        //score display
        if (isPaused) {
            scoreMessage = "Your score is: ";
            stringstream ss;
            ss << score;
            ss >> scoreString;
            if(scale->getHealth() == 100){
                pointMessage = ", and you are at your full health.";
            }
            else{
                pointMessage = ", sadly you are not at your full health.";
            }
            scoreMessage = scoreMessage + scoreString + pointMessage;
            XFillRectangle(xInfo.display, xInfo.pixmap, xInfo.gc[0], 180, 200, 245, 10);
            XDrawImageString(xInfo.display, xInfo.pixmap, xInfo.gc[1], 190, 210, scoreMessage.c_str(), scoreMessage.length());
            }
        
    }
    
    void stop(){
        distance_x = 0;
        isRunning = false;
    }
    
    
    void updateXCoordsMovingRight(){
        distance_x = marioRunningSpeed / 10;
        body_x = body_x + distance_x;
        leftLeg_x = leftLeg_x + distance_x;
        rightLeg_x = rightLeg_x + distance_x;
        head_x = head_x + distance_x;
        eyeLookRight_x = eyeLookRight_x + distance_x;
        mouthLookRight_x = mouthLookRight_x + distance_x;
        
    }
    
    void updateXCoordsMovingLeft(){
        distance_x = marioRunningSpeed / 10;
        body_x = body_x - distance_x;
        leftLeg_x = leftLeg_x - distance_x;
        rightLeg_x = rightLeg_x - distance_x;
        head_x = head_x - distance_x;
        eyeLookLeft_x = eyeLookLeft_x - distance_x;
        mouthLookLeft_x = mouthLookLeft_x - distance_x;
    }
    
    
    void updateXCoordsJumpingRight(){
        distance_x = marioJumpingSpeed * delta;
        body_x = body_x + distance_x;
        leftLeg_x = leftLeg_x + distance_x;
        rightLeg_x = rightLeg_x + distance_x;
        head_x = head_x + distance_x;
        eyeLookRight_x = eyeLookRight_x + distance_x;
        mouthLookRight_x = mouthLookRight_x + distance_x;
        
    }
    
    void updateXCoordsJumpingLeft(){
        distance_x = marioJumpingSpeed * delta;
        body_x = body_x - distance_x;
        leftLeg_x = leftLeg_x - distance_x;
        rightLeg_x = rightLeg_x - distance_x;
        head_x = head_x - distance_x;
        eyeLookLeft_x = eyeLookLeft_x - distance_x;
        mouthLookLeft_x = mouthLookLeft_x - distance_x;
    }
    
    void updateYCoords(){
        
        distance_y = (tan(jumpAngle) * distance_x);
        if ((rightLeg_x < (jumpMidpoint + startDestination) && facingRight) || (leftLeg_x > (startDestination - jumpMidpoint) && !facingRight)) {
            body_y = body_y - distance_y;
            leftLeg_y = leftLeg_y - distance_y;
            rightLeg_y = rightLeg_y - distance_y;
            head_y = head_y - distance_y;
            if(facingRight){
                eyeLookRight_y = eyeLookRight_y - distance_y;
                mouthLookRight_y = mouthLookRight_y - distance_y;
            }
            else{
                eyeLookLeft_y = eyeLookLeft_y - distance_y;
                mouthLookLeft_y = mouthLookLeft_y - distance_y;
            }
            
        }
        
        else{
            
            if((rightLeg_x < jumpDestination && facingRight) || (leftLeg_x > jumpDestination && !facingRight)){
                //when to stop moving down once started jump (ie. when mario's feet collide with topmost block)
                int block = (jumpDestination / 100);
                block = block * 100;
                list<Block *>::const_iterator begin = collisionList.begin();
                list<Block *>::const_iterator end = collisionList.end();
                bool firstOneFound = true; //first one found will be the topmost on stack of blocks, collision list is constructed with this in mind
                while( begin != end) {
                    Block *b = *begin;
                    if ((b -> getX()) == block && firstOneFound) {
                        if (rightLeg_y > (b -> getStopMovingDown())) {
                            distance_y = 0;
                            //readjust mario
                         
                            body_y = b->getY() - 67;
                            leftLeg_y = b->getY()- 20;
                            rightLeg_y = b->getY()-20;
                            head_y = b->getY() - 96;
                            eyeLookLeft_y = b->getY() - 93;
                            mouthLookLeft_y = b->getY() - 167;
                            eyeLookRight_y = body_y - 25;
                            mouthLookRight_y = body_y - 20;
                            eyeLookLeft_y = body_y - 25;
                            mouthLookLeft_y = body_y - 20;

                        }
                        firstOneFound = false;
                    }
                    begin++;
                }
                
                body_y = body_y + distance_y;
                leftLeg_y = leftLeg_y + distance_y;
                rightLeg_y = rightLeg_y + distance_y;
                head_y = head_y + distance_y;
                if(facingRight){
                    eyeLookRight_y = eyeLookRight_y + distance_y;
                    mouthLookRight_y = mouthLookRight_y + distance_y;
                }
                else{
                    eyeLookLeft_y = eyeLookLeft_y + distance_y;
                    mouthLookLeft_y = mouthLookLeft_y + distance_y;
                }
            }
        }
    }
    
    
    
    //when walk off an elevated block should land at ground level
    void updateYCoordsMoving(){
        list<Block *>::const_iterator begin = collisionList.begin();
        list<Block *>::const_iterator end = collisionList.end();
        int block;
        if (facingRight) {
            block = (leftLeg_x - 40) / 100;
            block = (block * 100);
        }
        else{
            block = (rightLeg_x + 40) / 100;
            block = (block * 100);
        }
        
        while( begin != end) {
            Block *b = *begin;
            if ((b -> getX()) == block && b-> getY() != 550 ) { //if block is not at ground level then when walk off the block, mario should drop down to ground level
                if (rightLeg_x > (b -> getX() + 107.36)) {
                    body_y = 483;
                    leftLeg_y = 533;
                    rightLeg_y = 533;
                    head_y = 453;
                    eyeLookRight_y = 460;
                    eyeLookLeft_y = 460;
                    mouthLookLeft_y = 466;
                    mouthLookRight_y = 466;
                }
                else if(leftLeg_x < (b -> getX() - 10.4)){
                    body_y = 483;
                    leftLeg_y = 533;
                    rightLeg_y = 533;
                    head_y = 453;
                    eyeLookRight_y = 460;
                    eyeLookLeft_y = 460;
                    mouthLookLeft_y = 466;
                    mouthLookRight_y = 466;
                }
            }
            begin++;
        }
    }

    
    
    void moveRight(){

        if (isPaused) {
            distance_x = 0;
        }
        else if (facingRight) {
            
            if (detectCollision()) {
                
                isRunning = false;
                distance_x = 0;
            }
            else{
                
                isRunning = true;
                updateXCoordsMovingRight();
                updateYCoordsMoving();
            }
        }
        //else facing left currently so now turn to face right
        else{
            
            isRunning = false;
            facingRight = true;
            eyeLookRight_x = eyeLookLeft_x + 12;
            mouthLookRight_x = mouthLookLeft_x + 29;
            eyeLookRight_y = body_y - 25;
            mouthLookRight_y = body_y - 20;
        }
    }
    
    void moveLeft(){
        if (isPaused){
            distance_x = 0;
        }
        //facing left already so now the extra left arrow key should make mario run
        else if(!facingRight){
            
            //see if collided with any block
            if (detectCollision()) {
                
                isRunning = false;
                distance_x = 0;
            }
            
            else{
                isRunning = true;
                updateXCoordsMovingLeft();
                updateYCoordsMoving();
            }
            
        }
        else{
            //must have previously been facing right, so by clicking left arrow, Mario now faces left, hence facingRight is set to false
            isRunning = false;
            facingRight = false;
            eyeLookLeft_x = eyeLookRight_x - 12;
            mouthLookLeft_x = mouthLookRight_x - 29;
            eyeLookLeft_y = body_y - 25;
            mouthLookLeft_y = body_y - 20;
        }
    }
    
    
    
    bool detectCollision(){
        list<Block *>::const_iterator begin = collisionList.begin();
        list<Block *>::const_iterator end = collisionList.end();
        
        if (facingRight) {
            while( begin != end ) {
                Block *b = *begin;
                if (rightLeg_x > (b -> getCollisionFromRight_x1()) && rightLeg_x < (b -> getCollisionFromRight_x2()) && b -> getCollisionFromTop() == rightLeg_y|| rightLeg_x > EndOfLeftScreenWalk) {
                    return true;
                }
                begin++;
            }
            
            
            return false;
        }
        else
        {
            while( begin != end ) {
                Block *b = *begin;
                
                if (leftLeg_x < (b -> getCollisionFromLeft_x1()) && leftLeg_x > (b -> getCollisionFromLeft_x2()) && b -> getCollisionFromTop() == rightLeg_y || leftLeg_x < EndOfRightScreenWalk) {
                    return true;
                }
                begin++;
            }
            
            return false;
            
        }
    }
    
    void detectCoinAndSpike(){
        int points = scale -> getPoints();
        int healthValue =  scale -> getHealth();
        
        if(facingRight){
            if (rightLeg_x > 145.6 && rightLeg_x < 184.38 && rightLeg_y > 510 && collectedObjects[0] == false) {
                dList.remove(money);
                score++;
                points = points + 50;
                collectedObjects[0] = true;
            }
            else if(rightLeg_x > 544.2 && rightLeg_x < 583.01 && rightLeg_y > 510 && collectedObjects[1] == false){
                dList.remove(money2);
                score++;
                points = points + 50;
                collectedObjects[1] = true;
            }
            else if(rightLeg_x < 382.2 && rightLeg_x > 309.41 && rightLeg_y > 475 && collectedObjects[2] == false){
                dList.remove(spike);
                score--;
                points = points - 50;
                collectedObjects[2] = true;
            }
            else if(rightLeg_x < 780.07 && rightLeg_x > 721.91 && rightLeg_y > 420 && rightLeg_y < 440 && collectedObjects[3] == false){
                dList.remove(money3);
                score++;
                points = points + 50;
                collectedObjects[3] = true;
            }
            
            //detect heart
            else if (head_x > 120 && head_x < 230 && head_y > 335 && head_y < 360 && collectedObjects[5] == false){
                dList.remove(heart);
                healthValue = healthValue + 50;
                collectedObjects[5] = true;
            }
            
        }
        else {
            if (leftLeg_x > 146.38 && leftLeg_x < 184.49 && leftLeg_y > 510 && collectedObjects[0] == false) {
                dList.remove(money);
                score++;
                points = points + 50;
                collectedObjects[0] = true;
            }
            else if(leftLeg_x > 546.56 && leftLeg_x < 584.93 && leftLeg_y > 510 && collectedObjects[1] == false){
                dList.remove(money2);
                score++;
                points = points + 50;
                collectedObjects[1] = true;
            }
            else if(leftLeg_x < 360.13 && leftLeg_x > 309.41 && rightLeg_y > 475 && collectedObjects[2] == false){
                dList.remove(spike);
                score--;
                points = points - 50;
                collectedObjects[2] = true;
            }
            else if(leftLeg_x < 65.60 && leftLeg_x > 13.26 && leftLeg_y > 370 && leftLeg_y < 401 && collectedObjects[4] == false){
                dList.remove(money4);
                score++;
                points = points + 50;
                collectedObjects[4] = true;
            }
            //detect heart
            else if (head_x > 126 && head_x < 230 && head_y > 335 && head_y < 360 && collectedObjects[5] == false){
                dList.remove(heart);
                healthValue = healthValue + 50;
                collectedObjects[5] = true;
            }

            
        }
        
        scale -> setPoints(points);
        scale -> setHealth(healthValue);

    }
    
    double getJumpAngle(){
        //calculate angle
        if (facingRight) {
            startDestination = rightLeg_x;
        }
        else{
            startDestination = leftLeg_x;
        }
        jumpMidpoint = ((jumpDestination - startDestination)/2);
        if (!facingRight) {
            jumpMidpoint = -jumpMidpoint;
        }
        jumpAngle = atan(heightJumped/jumpMidpoint);
        return jumpAngle;
    }
    
    double findJumpOverPos(){
        //facing right
        int leg_x;
        if(facingRight){
            leg_x = rightLeg_x;
        }
        else{
            leg_x = leftLeg_x;
        }
        if ((leg_x/100) == 0) {
            if (facingRight){
                return 250;
            }
            else{
                return 50;
            }
            
        }
        else if((leg_x/100) == 1){
            if(facingRight){
                return 350;
            }
            else{
                return 50;
            }
        }
        else if((leg_x/100) == 2){
            if(facingRight){
                return 450;
            }
            else{
                return 50;
            }
        }
        else if((leg_x/100) == 3){
            if(facingRight){
                return 550;
            }
            else{
                return 150;
            }
        }
        else if((leg_x/100) == 4){
            if(facingRight){
                return 650;
            }
            else{
                return 250;
            }
        }
        else if((leg_x/100) == 5){
            if(facingRight){
                return 750;
            }
            else{
                return 350;
            }
        }
        else if((leg_x/100) == 6){
            if(facingRight){
                return 750; //can't jump further than window width
            }
            else{
                return 450;
            }
        }
        else if((leg_x/100) == 7){
            if(facingRight){
                return 750; //can't jump further than window width
            }
            else{
                return 550;
            }
        }
    }
    
    double findJumpNextPos(){
        //facing right
        int leg_x;
        if(facingRight){
            leg_x = rightLeg_x;
        }
        else{
            leg_x = leftLeg_x;
        }
        if ((leg_x/100) == 0) {
            if (facingRight){
                return 150;
            }
            else{
                return 50;
            }
            
        }
        else if((leg_x/100) == 1){
            if(facingRight){
                return 250;
            }
            else{
                return 50;
            }
        }
        else if((leg_x/100) == 2){
            if(facingRight){
                return 350;
            }
            else{
                return 150;
            }
        }
        else if((leg_x/100) == 3){
            if(facingRight){
                return 450;
            }
            else{
                return 250;
            }
        }
        else if((leg_x/100) == 4){
            if(facingRight){
                return 550;
            }
            else{
                return 350;
            }
        }
        else if((leg_x/100) == 5){
            if(facingRight){
                return 650;
            }
            else{
                return 450;
            }
        }
        else if((leg_x/100) == 6){
            if(facingRight){
                return 750; //can't jump further than window width
            }
            else{
                return 550;
            }
        }
        else if((leg_x/100) == 7){
            if(facingRight){
                return 750; //can't jump further than window width
            }
            else{
                return 650;
            }
        }
        
    }
    
    
    
    void setJumpStats(){
        if(isRunning){
            jumpDestination = findJumpOverPos();
        }
        else{
            jumpDestination = findJumpNextPos();
        }
        jumpAngle = getJumpAngle();
    }
    
    
    void jump(){
        //jump over the next right block
        if (facingRight) {
            if (rightLeg_x < jumpDestination) {
                isJumping = true;
                if(detectCollision()){
                    distance_x = 0;
                    distance_y = 0;
                }
                else if(isPaused){
                    distance_x = 0;
                    distance_y = 0;
                }
                else{
                    updateXCoordsJumpingRight();
                    updateYCoords();
                }
            }
            
            else{
                isJumping = false;
                list<Block *>::const_iterator begin = collisionList.begin();
                list<Block *>::const_iterator end = collisionList.end();
                list<int> notEqualToComparisons;   // named this way because before had hardcoded: if (jumpDestination != elevated block postn pixels){ put back to ground level }
                
                while( begin != end ) {
                    Block *b = *begin;
                    if ((b -> getY()) < 550) { //find all corresponding elevated blocks jump destination positions and put into list
                        
                        notEqualToComparisons.push_front(b -> getX() + 50);
                    }
                    begin++;
                }
                bool goBackToGround = true;
                list<int>::const_iterator begin2 = notEqualToComparisons.begin();
                list<int>::const_iterator end2 = notEqualToComparisons.end();
                while( begin2 != end2 ) {
                    if(jumpDestination == *begin2){ //check if current jump destination matches any elevated position destinations, if it does we don't want to go back to ground
                        goBackToGround = false;
                    }
                    begin2++;
                }
                if(goBackToGround){
                    body_y = 483;
                    leftLeg_y = 533;
                    rightLeg_y = 533;
                    head_y = 453;
                    eyeLookRight_y = 460;
                    eyeLookLeft_y = 460;
                    mouthLookLeft_y = 466;
                    mouthLookRight_y = 466;
                }
            }
        }
        //jump over next left block
        else{
            
            if (leftLeg_x > jumpDestination) {
                isJumping = true;
                if(detectCollision()){
                    distance_x = 0;
                    distance_y = 0;
                }
                else if(isPaused){
                    distance_x = 0;
                    distance_y = 0;
                }
                else{
                    updateXCoordsJumpingLeft();
                    updateYCoords();
                }
            }
            //past destination point in the jump
            else{
                isJumping = false;
                
                list<Block *>::const_iterator begin = collisionList.begin();
                list<Block *>::const_iterator end = collisionList.end();
                list<int> notEqualToComparisons;
                
                while( begin != end ) {
                    Block *b = *begin;
                    if ((b -> getY()) < 550) {
                        
                        notEqualToComparisons.push_front(b -> getX() + 50);
                    }
                    begin++;
                }
                bool goBackToGround = true;
                list<int>::const_iterator begin2 = notEqualToComparisons.begin();
                list<int>::const_iterator end2 = notEqualToComparisons.end();
                while( begin2 != end2 ) {
                    if(jumpDestination == *begin2){
                        
                        goBackToGround = false;
                        
                    }
                    begin2++;
                }
                
                if(goBackToGround){
                    body_y = 483;
                    leftLeg_y = 533;
                    rightLeg_y = 533;
                    head_y = 453;
                    eyeLookRight_y = 460;
                    eyeLookLeft_y = 460;
                    mouthLookLeft_y = 466;
                    mouthLookRight_y = 466;
                }
            }
        }
        
        detectCoinAndSpike();
        
    }
    
    
    
    Mario(){
        facingRight = true;
        body_x = 20;
        body_y = 483;
        leftLeg_x = 20;
        leftLeg_y = 533;
        rightLeg_x = 37;
        rightLeg_y = 533;
        head_x = 16;
        head_y = 453;
        eyeLookRight_x = 35;
        eyeLookRight_y = 460;
        eyeLookLeft_y = 460;
        mouthLookLeft_y = 466;
        mouthLookRight_x = 43;
        mouthLookRight_y = 466;
        heightJumped = 150; //maximum y distance travelled during the jump, chosen aribitrarily
        collectedObjects[0] = false;
        collectedObjects[1] = false;
        collectedObjects[2] = false;
        collectedObjects[3] = false;
        collectedObjects[4] = false;
        collectedObjects[5] = false;        
    }
    
private:
    bool facingRight;
    double distance_x;
    double distance_y;
    double body_x;
    double body_y;
    double leftLeg_x;
    double leftLeg_y;
    double rightLeg_x;
    double rightLeg_y;
    double head_x;
    double head_y;
    double eyeLookRight_x;
    double eyeLookRight_y;
    double eyeLookLeft_x;
    double eyeLookLeft_y;
    double mouthLookRight_x;
    double mouthLookRight_y;
    double mouthLookLeft_x;
    double mouthLookLeft_y;
    double heightJumped;
    double jumpDestination;
    double jumpAngle;
    double jumpMidpoint;
    double startDestination;
    bool collectedObjects[6]; //ordered according to objects seen on screen from left to right
};
Mario *mario = new Mario();







//FUNCTIONS

void error( string str ) {
    cerr << str << endl;
    exit(0);
}

//initX does basic initialization
void initX(int argc, char *argv[], XInfo &xInfo) {
    
    xInfo.display = XOpenDisplay(""); // open display (using DISPLAY env var)
    if ( !xInfo.display )	{
		error( "Can't open display." );
	}
    
    xInfo.screen = DefaultScreen(xInfo.display);
    unsigned long white, black;
    white = XWhitePixel( xInfo.display, xInfo.screen );
	black = XBlackPixel( xInfo.display, xInfo.screen );
    XSizeHints hints;
    hints.width = 800;
    hints.height = 600;
    hints.x = 10;
    hints.y = 10;
    hints.flags = PPosition | PSize;
    xInfo.window = XCreateSimpleWindow(
                                       xInfo.display,
                                       DefaultRootWindow(xInfo.display),
                                       hints.x, hints.y,
                                       hints.width,
                                       hints.height,
                                       Border,
                                       black,
                                       white
                                       );
    
    
    XSetStandardProperties(
                           xInfo.display,	// display containing the window
                           xInfo.window,	// window whose properties are set
                           "Mario",		    // window's title
                           "Mario",		    // icon's title
                           None,			// pixmap for the icon
                           argv, argc,		// applications command line args
                           &hints           // size hints for the window
                           );
    
    
    //graphics context info here
    int i = 0;
	xInfo.gc[i] = XCreateGC(xInfo.display, xInfo.window, 0, 0);
	XSetForeground(xInfo.display, xInfo.gc[i], BlackPixel(xInfo.display, xInfo.screen));
	XSetBackground(xInfo.display, xInfo.gc[i], WhitePixel(xInfo.display, xInfo.screen));
	XSetFillStyle(xInfo.display, xInfo.gc[i], FillSolid);
	XSetLineAttributes(xInfo.display, xInfo.gc[i],
                       1, LineSolid, CapButt, JoinRound);
    
	// Reverse Video
	i = 1;
	xInfo.gc[i] = XCreateGC(xInfo.display, xInfo.window, 0, 0);
	XSetForeground(xInfo.display, xInfo.gc[i], WhitePixel(xInfo.display, xInfo.screen));
	XSetBackground(xInfo.display, xInfo.gc[i], BlackPixel(xInfo.display, xInfo.screen));
	XSetFillStyle(xInfo.display, xInfo.gc[i], FillSolid);
	XSetLineAttributes(xInfo.display, xInfo.gc[i],
                       1, LineSolid, CapButt, JoinRound);
    
    
    i = 2;
    xInfo.gc[i] = XCreateGC(xInfo.display, xInfo.window, 0, 0);
    xInfo.colorMap = DefaultColormap(xInfo.display, xInfo.screen);
    xInfo.xColor.red = 52000;
    xInfo.xColor.green = 9000;
    xInfo.xColor.blue = 9000;
    xInfo.xColor.flags = DoRed| DoGreen | DoBlue;
    XAllocColor(xInfo.display, xInfo.colorMap, &xInfo.xColor);
    XSetForeground(xInfo.display,xInfo.gc[2],xInfo.xColor.pixel);
    
    //pixmap for double buffer
    int depth = DefaultDepth(xInfo.display, DefaultScreen(xInfo.display)); //'depth' specifies the number of bits used to represent a color
	xInfo.pixmap = XCreatePixmap(xInfo.display, xInfo.window, hints.width, hints.height, depth);
    
    xInfo.width = hints.width;
	xInfo.height = hints.height;
    
    //details which events should the program listen to TODO
    XSelectInput(
                 xInfo.display, xInfo.window,
                 KeyPressMask |
                 KeyReleaseMask |
                 ExposureMask | //exposure occurs when window first appears and when the window is raised/resized
                 LeaveWindowMask |
                 StructureNotifyMask // for resize events
                 );
    
    //put the window on screen
    XMapRaised(xInfo.display, xInfo.window);
    XFlush(xInfo.display);
    sleep(2);
}


void repaint( XInfo &xInfo) {
    list<Displayable *>::const_iterator begin = dList.begin();
	list<Displayable *>::const_iterator end = dList.end();
    //needed to clear out the pixmap of any garbage data
    XFillRectangle(xInfo.display, xInfo.pixmap, xInfo.gc[1],
                   0, 0, xInfo.width, xInfo.height);
    
    while( begin != end ) {
		Displayable *d = *begin;
		d->paint(xInfo); // the displayables know about the pixmap
		begin++;
	}
    
    // copy buffer to window, reposition pixmap if need be after resizing
    double shiftRightAmount = 0;
    double shiftDownAmount = 0;
    
    if (xInfo.width >= 800) {
        shiftRightAmount = (xInfo.width - 800)/2;
    }
    
    if (xInfo.height >= 600) {
        shiftDownAmount = (xInfo.height - 600)/2;
    }
    
    
	XCopyArea(xInfo.display, xInfo.pixmap, xInfo.window, xInfo.gc[0],
              0, 0, xInfo.width, xInfo.height,  // region of pixmap to copy
              0 + shiftRightAmount, 0 + shiftDownAmount); // position to put top left corner of pixmap in window
	XFlush( xInfo.display );
}



Text* directions;
Text* directions2;
Text* directions3;
void startScreen(XInfo xInfo){
    directions = new Text(165, 236, "Press SPACE to start/pause, q to quit, left/right arrows to move.");
    directions2 = new Text(100, 256, "Press j while standing still to jump to next block, j while running to jump over next block.");
    directions3 = new Text(37, 276, "Collect the circular coins and heart. Avoid the spikes, they make you loose a coin. Bars on top left track progress.");
    dList.push_front(directions);
    dList.push_front(directions2);
    dList.push_front(directions3);

    repaint(xInfo);
}




void addTerrainToList(){
    // Add stuff to paint to the display list, higher stacked objs are placed in front of their lower counterpart
    dList.push_front(blockA);
    dList.push_front(blockL);
    dList.push_front(blockB);
    dList.push_front(blockC);
    dList.push_front(blockD);
    dList.push_front(blockE);
    dList.push_front(blockF);
    dList.push_front(blockG);
    dList.push_front(blockH);
    dList.push_front(blockI);
    dList.push_front(blockJ);
    dList.push_front(blockK);
    
    dList.push_front(money);
    dList.push_front(money2);
    dList.push_front(money3);
    dList.push_front(money4);
    
    dList.push_front(sun);
    dList.push_front(spike);
    dList.push_front(mario);
    dList.push_front(heart);
    dList.push_front(scale);

}


void addObjectsToCollisionList(){
    //topmost block in case where blocks are stacked should be in front of the block below it with respect to position in the list
    collisionList.push_front(blockI);
    collisionList.push_front(blockJ);
    collisionList.push_front(blockK);
    collisionList.push_front(blockH);
    collisionList.push_front(blockG);
    collisionList.push_front(blockF);
    collisionList.push_front(blockD);
    collisionList.push_front(blockE);
    collisionList.push_front(blockC);
    collisionList.push_front(blockB);
    //collisionList.push_front(blockA);
    collisionList.push_front(blockL);
}

Text *screenSizeMessage = NULL;
void checkMinimumWindowSize(XConfigureEvent xce, XInfo xInfo){
    //display error message if attempt to resize window to less than 800 by 600, clear message when proper dimensions are reached
    if (xce.width < 800 || xce.height < 600) {
        screenSizeMessage = new Text(300, 300, "Window size is too small!");
        dList.push_front(screenSizeMessage);
    }
    
    if(xce.width >= 800 && xce.height >=600 && screenSizeMessage!= NULL){
        dList.clear(); //had to clear it because was not working with a simple remove(screenSizeMessage) because that obj was pushed multiple times into dList
        screenSizeMessage = NULL;
        if(isStartScreen || isPaused){
            startScreen(xInfo);
        }
        if (!isStartScreen) {
            addTerrainToList();
        }
    }
}




// update width and height when window is resized
void handleResize(XInfo &xInfo, XEvent &event) {
	XConfigureEvent xce = event.xconfigure;
	//fprintf(stderr, "Handling resize  w=%d  h=%d\n", xce.width, xce.height);
    
    checkMinimumWindowSize(xce, xInfo);
    
	if (xce.width != xInfo.width || xce.height != xInfo.height) {
        XFreePixmap(xInfo.display, xInfo.pixmap);
		int depth = DefaultDepth(xInfo.display, DefaultScreen(xInfo.display));
		xInfo.pixmap = XCreatePixmap(xInfo.display, xInfo.window, xce.width, xce.height, depth);
		xInfo.width = xce.width;
		xInfo.height = xce.height;
	}
}


void handleKeyPress(XInfo &xInfo, XEvent &event) {
    KeySym key;
    char text[BufferSize];
    int i = XLookupString(
                          (XKeyEvent *)&event, 	// the keyboard event
                          text, 				// buffer when text will be written
                          BufferSize, 			// size of the text buffer
                          &key, 				// workstation-independent key symbol
                          NULL                  // pointer to a composeStatus structure (unused)
                          );
    
    
    if ( i == 1) {
   		//printf("Got key press -- %c\n", text[0]);
        //q key case
        if (text[0] == 'q') {
			error("Terminating normally.");
		}
        //j key case
        else if(text[0] == 'j'){
            if(!isJumping){  // need this extra if statement because we don't want mario to try and make jump while already in the middle of a jump, this would imply a flying mario!
                mario -> setJumpStats();
                mario -> jump();
                
            }
        }
    }
    
    
    //space key case
    if(XLookupKeysym(&event.xkey, 0) == XK_space){
        isPaused = !isPaused;
        //first time press space we are entering the game (first time isStartScreen and isPaused are true)
        //so don't enter if condition
        if (!isStartScreen && isPaused) {
            
            //pause game via isPaused var in setDistance function
            
            startScreen(xInfo); //display instructions again
            
        }
        //space pressed but no longer paused because we are currently in start screen
        else{
            //clear instructions
            dList.remove(directions);
            dList.remove(directions2);
            dList.remove(directions3);

            sun -> setDistance();
        }
        
        isStartScreen = false; //we are past the game's start screen, so set it as false always from now
    }
    
    
    //right arrow key case
    else if (XLookupKeysym(&event.xkey, 0) == XK_Right){
        if (startSuccessful) { //needed to ensure that mario does not speed off the screen because delta has not be set yet
            if(!isJumping){      
                mario -> moveRight();
                //see if collided with coin or spike
                mario -> detectCoinAndSpike();
            }
        }    
        
    }
    
    
    //left arrow key case
    else if (XLookupKeysym(&event.xkey, 0) == XK_Left){
        if(!isJumping){ 
            mario -> moveLeft();
            //see if collided with coin or spike
            mario -> detectCoinAndSpike();
        }
    }
	
}


void handleKeyRelease(){
    mario -> stop();
}




bool firstHandleAnimationCall = true; // initially set to true, after first time in handleSunAnimation it is set to false
void handleSunAnimation(XInfo &xInfo) {
    if(firstHandleAnimationCall){
        firstHandleAnimationCall = false;
        return; //because first time it is called we haven't gotten 2 frames yet, so delta is not correct
    }
    sun -> setDistance();
    sun -> move(xInfo);
}


// get microseconds
unsigned long now() {
	timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec * 1000000.0 + tv.tv_usec;
}

void eventLoop(XInfo &xInfo) {
    
    XEvent event;
    unsigned long endTime;
    unsigned long previousRepaintCall = 0;
    startScreen(xInfo);
    while (isStartScreen) {
        if (XPending(xInfo.display) > 0) {
            XNextEvent( xInfo.display, &event );
            switch (event.type) {
                case KeyPress:
                    handleKeyPress(xInfo, event);
                    break;
                case ConfigureNotify:
                    handleResize(xInfo, event);
                    break;
            }
        }
        endTime = now();
        if((endTime - previousRepaintCall) > 1000000/FPS){
            delta = (endTime - previousRepaintCall)/1000000.0;
            repaint(xInfo);
            previousRepaintCall = now();
        }
        else if (XPending(xInfo.display) == 0){
            usleep((1000000/FPS) - (endTime - previousRepaintCall)); //sleep for about 1/50th of a second minus elapsed time
        }
    }
    
    dList.remove(directions);
    dList.remove(directions2);
    dList.remove(directions3);

    addTerrainToList();
    addObjectsToCollisionList();
    
    while (true) {
        
        if (XPending(xInfo.display) > 0) {
            XNextEvent( xInfo.display, &event );
            //problem is that going through one event per frame
            switch (event.type) {
                case KeyPress:
                    handleKeyPress(xInfo, event);
                    break;
                case KeyRelease:
                    handleKeyRelease();
                case Expose:
                    break;
                case ConfigureNotify:
                    handleResize(xInfo, event);
                    break;
            }
        }
        
        //with the below code, we still get a chance to sleep however, we only do so when there are no pending events thus less likely to miss events
        
        //trying to decrease the lag of the program with the following lines of code
        endTime = now();
        //check if time elapsed since last cycle > 1000000/FPS
        if((endTime - previousRepaintCall) > 1000000/FPS){
            delta = (endTime - previousRepaintCall)/1000000.0;
            repaint(xInfo);
            handleSunAnimation(xInfo);
            if(isJumping){
                mario -> jump();
            }
            previousRepaintCall = now();
            startSuccessful = true; //can start moving right from start position now, since delta value is now set for this while loop
        }
        else if (XPending(xInfo.display) == 0){
            usleep((1000000/FPS) - (endTime - previousRepaintCall)); //sleep for about 1/50th of a second minus elapsed time
        }
    }
}




int main(int argc, char *argv[]) {
   cout<< "********************************************************************************" << "\n";
   cout<< "WELCOME, ENJOY THE GAME"<< "\n";
   cout<< "********************************************************************************" << "\n";
   
    
    marioRunningSpeed = atoi(argv[1]);
    marioJumpingSpeed = atoi(argv[2]);
    sunSpeed = atoi(argv[3]);
    FPS = atoi(argv[4]);
    
    XInfo xInfo;
    initX(argc, argv, xInfo);
    eventLoop(xInfo);
}
