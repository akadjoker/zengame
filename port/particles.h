// Enums
enum EmitterType
{
    EMITTER_CONTINUOUS,
    EMITTER_ONESHOT
};

// ============================================================================
// Particle Structure
// ============================================================================
struct Particle
{
    Vector2 pos = {0, 0};
    Vector2 vel = {0, 0};
    Vector2 acc = {0, 0}; // Aceleração (ex: gravidade)

    Color color = WHITE;
    Color startColor = WHITE;
    Color endColor = WHITE;

    float life = 0.0f;
    float maxLife = 1.0f;

    float size = 1.0f;
    float startSize = 1.0f;
    float endSize = 0.0f;

    float rotation = 0.0f;
    float angularVel = 0.0f;

    bool alive = false;
};

// ============================================================================
// Emitter Class
// ============================================================================
class Emitter
{
public:
    // Configuração básica
    EmitterType type;
    int graph;
    int layer;

    Vector2 pos;
    Vector2 dir;

    // Parâmetros de emissão
    float spread; // Cone de dispersão (radians)
    float rate;   // Partículas por segundo
    float speedMin;
    float speedMax;
    float particleLife;
    float lifetime; // Duração do emitter (oneshot)

    // Aparência
    Color colorStart;
    Color colorEnd;
    float sizeStart;
    float sizeEnd;

    // Física
    Vector2 gravity; // Aceleração constante
    float drag;      // Resistência do ar (0-1)

    // Rotação
    float rotationMin;
    float rotationMax;
    float angularVelMin;
    float angularVelMax;
    Rectangle spawnZone;

    // Rendering
    BlendMode blendMode;

    // Estado
    bool active;
    bool finished;

    // Partículas
    std::vector<Particle> particles;

    // Setters convenientes
    void setPosition(float x, float y) { pos = {x, y}; }
    void setDirection(float x, float y) { dir = {x, y}; }
    void setEmissionRate(float r) { rate = r; }
    void setLife(float life) { particleLife = life; }
    void setSpeedRange(float min, float max)
    {
        speedMin = min;
        speedMax = max;
    }
    void setSpread(float radians) { spread = radians; }
    void setColorCurve(Color start, Color end)
    {
        colorStart = start;
        colorEnd = end;
    }
    void setSizeCurve(float start, float end)
    {
        sizeStart = start;
        sizeEnd = end;
    }
    void setSpawnZone(float x, float y, float w, float h) { spawnZone = {x, y, w, h}; }
    void setLifeTime(float time) { lifetime = time; }
    void setGravity(float x, float y) { gravity = {x, y}; }
    void setDrag(float d) { drag = d; }
    void setRotationRange(float min, float max)
    {
        rotationMin = min;
        rotationMax = max;
    }
    void setAngularVelRange(float min, float max)
    {
        angularVelMin = min;
        angularVelMax = max;
    }
    void setBlendMode(BlendMode mode) { blendMode = mode; }
    void setLayer(int l) { layer = l; }

    // Getters
    int getAliveCount() const { return aliveCount; }
    int getMaxParticles() const { return (int)particles.size(); }
    bool isFinished() const { return finished; }
    Vector2 getPosition() const { return pos; }

private:
    float elapsed;
    float accumulator;
    int aliveCount;
    int firstDead;

    void emit();
    void emitAt(int index);

public:
    Emitter(EmitterType t, int gr, int maxParticles);
    ~Emitter() = default;

    void update(float dt);
    void draw();

    void burst(int count);
    void stop();
    void restart();
};

// ============================================================================
// Particle System Manager
// ============================================================================
class ParticleSystem
{
private:
    std::vector<Emitter *> emitters;

public:
    ParticleSystem();
    ~ParticleSystem();

    // Criação de emitters
    Emitter *spawn(EmitterType type, int graph, int maxParticles = 100);

    // Presets comuns
    Emitter *createExplosion(Vector2 pos, int graph, Color color);
    Emitter *createSmoke(Vector2 pos, int graph);
    Emitter *createFire(Vector2 pos, int graph);
    Emitter *createSparks(Vector2 pos, int graph, Color color);

    // Impactos e Colisões
    Emitter *createLandingDust(Vector2 pos, int graph, bool facingRight);
    Emitter *createWallImpact(Vector2 pos, int graph, bool hitFromLeft, float size_start, float size_end);
    Emitter *createWaterSplash(Vector2 pos, int graph);

    // Movimento do Player
    Emitter *createRunTrail(Vector2 pos, int graph,float size_start,float size_end);
    Emitter *createSpeedLines(Vector2 pos, int graph, Vector2 velocity);

    // Coleta e Power-ups
    Emitter *createCollectEffect(Vector2 pos, int graph, Color itemColor);
    Emitter *createPowerUpAura(Vector2 pos, int graph, Color auraColor);
    Emitter *createSparkle(Vector2 pos, int graph);
    Emitter *createMagicCast(Vector2 pos, int graph, Color magicColor);
    Emitter *createPortal(Vector2 pos, int graph);
    Emitter *createShieldHit(Vector2 pos, int graph, Vector2 hitDirection);

    // Dano e Combat
    Emitter *createBloodSplatter(Vector2 pos, int graph, Vector2 hitDirection);
    Emitter *createMuzzleFlash(Vector2 pos, int graph, Vector2 shootDirection);
    Emitter *createShellEjection(Vector2 pos, int graph, bool facingRight);
    Emitter *createBulletTracer(Vector2 startPos, Vector2 endPos, int graph);
    Emitter *createRicochet(Vector2 pos, int graph, Vector2 normal);

    // Ambiente
    Emitter *createRain(Vector2 pos, int graph, float width);
    Emitter *createFallingLeaves(Vector2 pos, int graph, float width);
    Emitter *createFootstepDust(Vector2 pos, int graph);
    Emitter *createDustCloud(Vector2 pos, int graph, float radius);


    void update(float dt);
    void cleanup();
    void draw();
    void clear();

    int getEmitterCount() const { return (int)emitters.size(); }
    int getTotalParticles() const;
};