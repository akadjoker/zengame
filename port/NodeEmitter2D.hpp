#pragma once
#include "Core.hpp"
#include "Node.hpp"
#include "Node2D.hpp"
#include "Graph.hpp"



struct Particle
{
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
    float Start;
    float End;
    float X;
    float Y;
    float VX;
    float VY;
    float Angle;
    float AngleDelta;
    float Scale;
    float Time;
    bool  Alive;
    bool trasform;
    Particle()
    {
        r = 255;
        g = 255;
        b = 255;
        a = 255;
        Start = 0;
        End = 0;
        X = 0;
        Y = 0;
        VX = 0;
        VY = 0;
        Angle = 0;
        AngleDelta = 0;
        Scale = 1;
        Time = 0;
        Alive = false;
        trasform = false;
    }

};


struct ColorKey
{
    unsigned char r;
    unsigned char g;
    unsigned char b;
    float time;
};

struct SizeKey
{
    float value;
    float time;
};

struct AlphaKey
{
    unsigned char  value;
    float time;
};

class NodeEmitter2D : public Node2D
{

    public:
    NodeEmitter2D(const std::string &graph, int numParticles,bool oneshot);
    NodeEmitter2D(const std::string &name, const std::string &graph, int numParticles,bool oneshot);
    virtual ~NodeEmitter2D();


    virtual void OnDraw(View *view) override;
    virtual void OnUpdate(double deltaTime) override;

    void SetColorEnd(  unsigned char r, unsigned char g, unsigned char b, float time );
    void SetColorStart(unsigned char r, unsigned char g, unsigned char b, float time );
    
    void SetSizeEnd(float size, float time );
    void SetSizeStart(float size, float time );

    void SetAlphaEnd(float alpha, float time );
    void SetAlphaStart(float alpha, float time );
    
    void SetFrequency( float freq );
    void SetStartZone( float x1, float y1, float x2, float y2 );
    void SetDirection( float vx, float vy );
    void SetSelocityRange( float v1, float v2 );
    void SetAngle(float angle );
    void SetAngleRad(float angle );
    void ResetCounter();
    
    void SetByMatrix(bool flag);

    float GetFrequency() { return Freq; }
    float GetDirectionX() { return VX; }
    float GetDirectionY() { return VY; }
    float GetAngle() { return Angle * 180 / PI; }
    float GetAngleRad() { return Angle; }
    float GetSize() { return Size; }
    float GetLife() { return Life; }

    bool IsFinished();
    bool IsReachEnd();

    void SetSize(float size );
    void SetMaxParticles(int max );
    void SetRotationRate( float a1, float a2);
    void SetRotationRateRad( float a1, float a2);
    void SetFaceDirection(bool flag );
    void Offset(float x, float y );
    void SetEmitterPosition(float x, float y );
    void SetLife( float time );
    void SetOneShot();


    private:
    Particle **Particles;
    float X;
    float Y;
    float VX;
    float VY;
    float Angle;
    float VMin;
    float VMax;
    float Size;
    ColorKey colorStart; 
    ColorKey colorEnd;
    SizeKey  sizeStart;
    SizeKey  sizeEnd;
    AlphaKey alphaStart;
    AlphaKey alphaEnd;
    int   Depth;
    float Life;
    bool InterpolateColor;
    float Freq;
    float NumStart;
    int CurrParticle;
    int NumParticles;
    bool OneShot;
    float AMin;
    float AMax;
    bool FaceDir;
    bool byMatrix{false};
    int MaxParticles;
    int _maxParticles;
    int EmittedParticles;

    bool SomeAlive;
    
    float StartX1;
    float StartY1;
    float StartX2;
    float StartY2;
    Graph *graph{nullptr};

    void UpdateNumParticles();

} ;