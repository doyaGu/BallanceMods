#pragma once

#include "VxVector.h"
#include "CKTypes.h"

#include "Serializable.h"

struct FrameHeader : Serializable {
    uint32_t version = 1;
    uint32_t checksum = 0;

    bool Serialize(std::ostream &out) const override;
    bool Deserialize(std::istream &in) override;
};

struct FrameEvent : Serializable {
    std::string eventType;  // Type of event (e.g., "collision", "custom_force")
    std::unordered_map<std::string, std::string> parameters; // Event-specific data

    bool Serialize(std::ostream &out) const override;
    bool Deserialize(std::istream &in) override;
};

struct PhysicsGlobalState : Serializable {
    VxVector gravity;
    float timeFactor = 1.0f;
    double deltaPSITime = 1 / 66.0;

    bool Serialize(std::ostream &out) const override;
    bool Deserialize(std::istream &in) override;
};

struct PhysicsProperties : Serializable {
    std::string name;
    std::string collisionGroup;
    VxVector shiftMassCenter;
    float mass = 1.0f;
    float friction = 0.7f;
    float elasticity = 0.4f;
    float linearDampening = 0.1f;
    float rotationalDampening = 0.1f;
    uint16_t flags = 0; // Bit flags: 0x1 - fixed, 0x2 - startFrozen, 0x4 - enableCollision, 0x8 - autoMassCenter

    [[nodiscard]] bool IsFixed() const { return flags & 0x1; }
    [[nodiscard]] bool IsStartFrozen() const { return flags & 0x2; }
    [[nodiscard]] bool IsCollisionEnabled() const { return flags & 0x4; }
    [[nodiscard]] bool IsAutoMassCenter() const { return flags & 0x8; }

    void SetFixed(bool value) { flags = value ? (flags | 0x1) : (flags & ~0x1); }
    void SetStartFrozen(bool value) { flags = value ? (flags | 0x2) : (flags & ~0x2); }
    void SetCollisionEnabled(bool value) { flags = value ? (flags | 0x4) : (flags & ~0x4); }
    void SetAutoMassCenter(bool value) { flags = value ? (flags | 0x8) : (flags & ~0x8); }

    bool Serialize(std::ostream &out) const override;
    bool Deserialize(std::istream &in) override;
};

struct PhysicsForce : Serializable {
    CK_ID object = 0;
    VxVector position;
    CK_ID positionRef = 0;
    VxVector direction = VxVector(0.0f, 0.0f, 1.0f);
    CK_ID directionRef = 0;
    float force = 10.0f;

    bool Serialize(std::ostream &out) const override;
    bool Deserialize(std::istream &in) override;
};

struct PhysicsImpulse : Serializable {
    CK_ID object = 0;
    VxVector position;
    CK_ID positionRef = 0;
    VxVector direction = VxVector(0.0f, 0.0f, 1.0f);
    CK_ID directionRef = 0;
    float impulse = 10.0f;
    bool dirAsPos = false;
    bool constant = false;

    bool Serialize(std::ostream &out) const override;
    bool Deserialize(std::istream &in) override;
};

struct PhysicsBallJoint : Serializable {
    CK_ID object1 = 0;
    CK_ID object2 = 0;
    VxVector position1;
    CK_ID referential1 = 0;

    bool Serialize(std::ostream &out) const override;
    bool Deserialize(std::istream &in) override;
};

struct PhysicsHinge : Serializable {
    CK_ID object1 = 0;
    CK_ID object2 = 0;
    CK_ID jointReferential = 0;
    bool limitations = false;
    float lowerLimit = -45.0f;
    float upperLimit = 45.0f;

    bool Serialize(std::ostream &out) const override;
    bool Deserialize(std::istream &in) override;
};

struct PhysicsSlider : Serializable {
    CK_ID object1 = 0;
    CK_ID object2 = 0;
    CK_ID axisPoint1 = 0;
    CK_ID axisPoint2 = 0;
    bool limitations = false;
    float lowerLimit = -1.0f;
    float upperLimit = 1.0f;

    bool Serialize(std::ostream &out) const override;
    bool Deserialize(std::istream &in) override;
};

struct PhysicsBuoyancy : Serializable {
    CK_ID object = 0;
    CK_ID surfacePoint1 = 0;
    CK_ID surfacePoint2 = 0;
    CK_ID surfacePoint3 = 0;
    CK_ID currentStartPoint = 0;
    CK_ID currentEndPoint = 0;
    float strength = 1.0f;
    float mediumDensity = 998.0f;
    float mediumDampFactor = 1.0f;
    float mediumFrictionFactor = 0.05f;
    float viscosity = 0.01f;
    float airplaneLikeFactor = 0.0f;
    float suctionFactor = 0.1f;

    bool Serialize(std::ostream &out) const override;
    bool Deserialize(std::istream &in) override;
};

struct PhysicsSpring : Serializable {
    CK_ID object1 = 0;
    VxVector position1;
    CK_ID referential1 = 0;
    CK_ID object2 = 0;
    VxVector position2;
    CK_ID referential2 = 0;
    float length = 1.0f;
    float constant = 1.0f;
    float linearDampening = 0.1f;
    float globalDampening = 0.1f;

    bool Serialize(std::ostream &out) const override;
    bool Deserialize(std::istream &in) override;
};

struct PhysicsCollDetection : Serializable {
    CK_ID object = 0;
    float minSpeed = 0.3f;
    float maxSpeed = 10.0f;
    float sleepAfterwards = 0.5f;
    int collisionID = 1;

    CK_ID entity = 0;
    float speed = 0.0f;
    VxVector collisionNormalWorld;
    VxVector positionWorld;
    bool useCollisionID = false;

    bool Serialize(std::ostream &out) const override;
    bool Deserialize(std::istream &in) override;
};

struct PhysicsContinuousContact : Serializable {
    CK_ID object = 0;
    float timeDelayStart = 0.1f;
    float timeDelayEnd = 0.1f;
    int numberGroupOutput = 5;

    bool Serialize(std::ostream &out) const override;
    bool Deserialize(std::istream &in) override;
};

struct PhysicsState : Serializable {
    VxVector position;
    VxVector orientation;
    VxVector linearVelocity;
    VxVector angularVelocity;
    uint32_t flags = 0; // Bit flags: 0x1 - isSleeping

    bool Serialize(std::ostream &out) const override;
    bool Deserialize(std::istream &in) override;
};

struct PhysicsFrame : Serializable {
    FrameHeader header;
    double currentTime = 0.0;
    double timeOfLastPSI = 0.0;
    double timeOfNextPSI = 0.0;
    PhysicsGlobalState envState;
    std::vector<PhysicsState> objects;

    bool Serialize(std::ostream &out) const override;
    bool Deserialize(std::istream &in) override;
};

struct Sector : Serializable {
    int id = 0;
    int frameStart = 0;
    int frameEnd = 0;
    VxVector startPosition;
    VxVector endPosition;
    std::vector<CK_ID> objects;

    bool Serialize(std::ostream &out) const override;
    bool Deserialize(std::istream &in) override;
};

struct InputState : Serializable {
    unsigned keyUp: 1 = 0;
    unsigned keyDown: 1 = 0;
    unsigned keyLeft: 1 = 0;
    unsigned keyRight: 1 = 0;
    unsigned keyShift: 1 = 0;
    unsigned keySpace: 1 = 0;
    unsigned keyQ: 1 = 0;
    unsigned keyEsc: 1 = 0;
    unsigned keyEnter: 1 = 0;

    bool Serialize(std::ostream &out) const override;
    bool Deserialize(std::istream &in) override;
};

struct GameFrame : Serializable {
    float deltaTime = 0.0f;
    InputState inputState = {};

    GameFrame() = default;
    explicit GameFrame(float delta) : deltaTime(delta) {}

    bool Serialize(std::ostream &out) const override;
    bool Deserialize(std::istream &in) override;
};

class TASRecord {
public:
    TASRecord() = default;
    TASRecord(std::string name, std::string path, bool legacy = false)
        : m_Name(std::move(name)), m_Path(std::move(path)), m_Legacy(legacy) {}

    bool operator==(const TASRecord &rhs) const { return m_Name == rhs.m_Name; }
    bool operator!=(const TASRecord &rhs) const { return !(rhs == *this); }

    bool operator<(const TASRecord &o) const { return m_Name < o.m_Name; }
    bool operator>(const TASRecord &rhs) const { return rhs < *this; }
    bool operator<=(const TASRecord &rhs) const { return !(rhs < *this); }
    bool operator>=(const TASRecord &rhs) const { return !(*this < rhs); }

    [[nodiscard]] const std::string &GetName() const { return m_Name; }
    void SetName(const std::string &name) { m_Name = name; }

    [[nodiscard]] const std::string &GetPath() const { return m_Path; }
    void SetPath(const std::string &path) { m_Path = path; }

    [[nodiscard]] const std::string &GetMapName() const { return m_MapName; }
    void SetMapName(const std::string &name) { m_MapName = name; }

    [[nodiscard]] bool IsLegacy() const { return m_Legacy; }
    [[nodiscard]] bool IsLoaded() const { return m_Loaded; }
    void Load();
    void Save() const;

    [[nodiscard]] bool IsPlaying() const { return m_FrameIndex < m_Frames.size(); }
    [[nodiscard]] bool IsFinished() const { return m_FrameIndex == m_Frames.size(); }

    [[nodiscard]] size_t GetFrameCount() const { return m_Frames.size(); }
    [[nodiscard]] size_t GetFrameIndex() const { return m_FrameIndex; }
    GameFrame &GetFrames() { return m_Frames[m_FrameIndex]; }
    GameFrame &GetFrame(size_t index) { return m_Frames[index]; }

    void NextFrame() { ++m_FrameIndex; }
    void PrevFrame() { --m_FrameIndex; }
    void ResetFrame() { m_FrameIndex = 0; }

    void NewFrame(const GameFrame &frame) {
        if (!m_Frames.empty())
            ++m_FrameIndex;
        m_Frames.emplace_back(frame);
    }

    [[nodiscard]] size_t GetSectorCount() const { return m_Sectors.size(); }
    [[nodiscard]] size_t GetSectorIndex() const { return m_SectorIndex; }
    [[nodiscard]] std::vector<Sector> &GetSectors() { return m_Sectors; }
    Sector &GetCurrentSector() { return m_Sectors[m_SectorIndex]; }

    void NextSector() { ++m_SectorIndex; }
    void PrevSector() { --m_SectorIndex; }
    void ResetSector() { m_SectorIndex = 0; }

    Sector &NewSector() {
        if (!m_Sectors.empty())
            ++m_SectorIndex;
        m_Sectors.emplace_back();
        return m_Sectors.back();
    }

    [[nodiscard]] uint32_t GetFlags() const { return m_Flags; }
    void SetFlags(uint32_t flags) { m_Flags = flags; }

    void Clear() {
        m_Loaded = false;
        m_FrameIndex = 0;
        m_Frames.clear();
        m_SectorIndex = 0;
        m_Sectors.clear();
    }

private:
    std::string m_Name;
    std::string m_Path;
    std::string m_MapName;
    bool m_Legacy = false;
    bool m_Loaded = false;
    size_t m_FrameIndex = 0;
    std::vector<GameFrame> m_Frames;
    size_t m_SectorIndex = 0;
    std::vector<Sector> m_Sectors;
    uint32_t m_Flags = 0;

    static constexpr uint32_t MAGIC_NUMBER = 0x534154; // "TAS" in reverse order
    static constexpr uint32_t VERSION = 1;

    static bool CompressData(const std::vector<uint8_t> &input, std::vector<uint8_t> &output);
    static bool DecompressData(const std::vector<uint8_t> &input, std::vector<uint8_t> &output);
};