#include "TASRecord.h"

#include <sstream>
#include <fstream>

#include <miniz.h>

#include "VectorStream.h"

bool FrameHeader::Serialize(std::ostream &out) const {
    return Write(out, version) && Write(out, checksum);
}

bool FrameHeader::Deserialize(std::istream &in) {
    return Read(in, version) && Read(in, checksum);
}

bool FrameEvent::Serialize(std::ostream &out) const {
    if (!WriteString(out, eventType)) return false;
    size_t paramCount = parameters.size();
    if (!Write(out, paramCount)) return false;
    for (const auto &[key, value] : parameters) {
        if (!WriteString(out, key) || !WriteString(out, value)) return false;
    }
    return true;
}

bool FrameEvent::Deserialize(std::istream &in) {
    if (!ReadString(in, eventType)) return false;
    size_t paramCount;
    if (!Read(in, paramCount)) return false;
    parameters.clear();
    for (size_t i = 0; i < paramCount; ++i) {
        std::string key, value;
        if (!ReadString(in, key) || !ReadString(in, value)) return false;
        parameters[key] = value;
    }
    return true;
}

bool PhysicsGlobalState::Serialize(std::ostream &out) const {
    return Write(out, gravity) &&
           Write(out, timeFactor) &&
           Write(out, deltaPSITime);
}

bool PhysicsGlobalState::Deserialize(std::istream &in) {
    return Read(in, gravity) &&
           Read(in, timeFactor) &&
           Read(in, deltaPSITime);
}

bool PhysicsProperties::Serialize(std::ostream &out) const {
    if (!WriteString(out, name)) return false;
    if (!WriteString(out, collisionGroup)) return false;
    return Write(out, shiftMassCenter) &&
           Write(out, mass) &&
           Write(out, friction) &&
           Write(out, elasticity) &&
           Write(out, linearDampening) &&
           Write(out, rotationalDampening) &&
           Write(out, flags);
}

bool PhysicsProperties::Deserialize(std::istream &in) {
    if (!ReadString(in, name)) return false;
    if (!ReadString(in, collisionGroup)) return false;
    return Read(in, shiftMassCenter) &&
           Read(in, mass) &&
           Read(in, friction) &&
           Read(in, elasticity) &&
           Read(in, linearDampening) &&
           Read(in, rotationalDampening) &&
           Read(in, flags);
}

bool PhysicsForce::Serialize(std::ostream &out) const {
    return Write(out, object) &&
           Write(out, position) &&
           Write(out, positionRef) &&
           Write(out, direction) &&
           Write(out, directionRef) &&
           Write(out, force);
}

bool PhysicsForce::Deserialize(std::istream &in) {
    return Read(in, object) &&
           Read(in, position) &&
           Read(in, positionRef) &&
           Read(in, direction) &&
           Read(in, directionRef) &&
           Read(in, force);
}

bool PhysicsImpulse::Serialize(std::ostream &out) const {
    return Write(out, object) &&
           Write(out, position) &&
           Write(out, positionRef) &&
           Write(out, direction) &&
           Write(out, directionRef) &&
           Write(out, impulse) &&
           Write(out, dirAsPos) &&
           Write(out, constant);
}

bool PhysicsImpulse::Deserialize(std::istream &in) {
    return Read(in, object) &&
           Read(in, position) &&
           Read(in, positionRef) &&
           Read(in, direction) &&
           Read(in, directionRef) &&
           Read(in, impulse) &&
           Read(in, dirAsPos) &&
           Read(in, constant);
}

bool PhysicsBallJoint::Serialize(std::ostream &out) const {
    return Write(out, object1) &&
           Write(out, object2) &&
           Write(out, position1) &&
           Write(out, referential1);
}

bool PhysicsBallJoint::Deserialize(std::istream &in) {
    return Read(in, object1) &&
           Read(in, object2) &&
           Read(in, position1) &&
           Read(in, referential1);
}

bool PhysicsHinge::Serialize(std::ostream &out) const {
    return Write(out, object1) &&
           Write(out, object2) &&
           Write(out, jointReferential) &&
           Write(out, limitations) &&
           Write(out, lowerLimit) &&
           Write(out, upperLimit);
}

bool PhysicsHinge::Deserialize(std::istream &in) {
    return Read(in, object1) &&
           Read(in, object2) &&
           Read(in, jointReferential) &&
           Read(in, limitations) &&
           Read(in, lowerLimit) &&
           Read(in, upperLimit);
}

bool PhysicsSlider::Serialize(std::ostream &out) const {
    return Write(out, object1) &&
           Write(out, object2) &&
           Write(out, axisPoint1) &&
           Write(out, axisPoint2) &&
           Write(out, limitations) &&
           Write(out, lowerLimit) &&
           Write(out, upperLimit);
}

bool PhysicsSlider::Deserialize(std::istream &in) {
    return Read(in, object1) &&
           Read(in, object2) &&
           Read(in, axisPoint1) &&
           Read(in, axisPoint2) &&
           Read(in, limitations) &&
           Read(in, lowerLimit) &&
           Read(in, upperLimit);
}

bool PhysicsBuoyancy::Serialize(std::ostream &out) const {
    return Write(out, object) &&
           Write(out, surfacePoint1) &&
           Write(out, surfacePoint2) &&
           Write(out, surfacePoint3) &&
           Write(out, currentStartPoint) &&
           Write(out, currentEndPoint) &&
           Write(out, strength) &&
           Write(out, mediumDensity) &&
           Write(out, mediumDampFactor) &&
           Write(out, mediumFrictionFactor) &&
           Write(out, viscosity) &&
           Write(out, airplaneLikeFactor) &&
           Write(out, suctionFactor);
}

bool PhysicsBuoyancy::Deserialize(std::istream &in) {
    return Read(in, object) &&
           Read(in, surfacePoint1) &&
           Read(in, surfacePoint2) &&
           Read(in, surfacePoint3) &&
           Read(in, currentStartPoint) &&
           Read(in, currentEndPoint) &&
           Read(in, strength) &&
           Read(in, mediumDensity) &&
           Read(in, mediumDampFactor) &&
           Read(in, mediumFrictionFactor) &&
           Read(in, viscosity) &&
           Read(in, airplaneLikeFactor) &&
           Read(in, suctionFactor);
}

bool PhysicsSpring::Serialize(std::ostream &out) const {
    return Write(out, object1) &&
           Write(out, position1) &&
           Write(out, referential1) &&
           Write(out, object2) &&
           Write(out, position2) &&
           Write(out, referential2) &&
           Write(out, length) &&
           Write(out, constant) &&
           Write(out, linearDampening) &&
           Write(out, globalDampening);
}

bool PhysicsSpring::Deserialize(std::istream &in) {
    return Read(in, object1) &&
           Read(in, position1) &&
           Read(in, referential1) &&
           Read(in, object2) &&
           Read(in, position2) &&
           Read(in, referential2) &&
           Read(in, length) &&
           Read(in, constant) &&
           Read(in, linearDampening) &&
           Read(in, globalDampening);
}

bool PhysicsCollDetection::Serialize(std::ostream &out) const {
    return Write(out, object) &&
           Write(out, minSpeed) &&
           Write(out, maxSpeed) &&
           Write(out, sleepAfterwards) &&
           Write(out, collisionID) &&
           Write(out, entity) &&
           Write(out, speed) &&
           Write(out, collisionNormalWorld) &&
           Write(out, positionWorld) &&
           Write(out, useCollisionID);
}

bool PhysicsCollDetection::Deserialize(std::istream &in) {
    return Read(in, object) &&
           Read(in, minSpeed) &&
           Read(in, maxSpeed) &&
           Read(in, sleepAfterwards) &&
           Read(in, collisionID) &&
           Read(in, entity) &&
           Read(in, speed) &&
           Read(in, collisionNormalWorld) &&
           Read(in, positionWorld) &&
           Read(in, useCollisionID);
}

bool PhysicsContinuousContact::Serialize(std::ostream &out) const {
    return Write(out, object) &&
           Write(out, timeDelayStart) &&
           Write(out, timeDelayEnd) &&
           Write(out, numberGroupOutput);
}

bool PhysicsContinuousContact::Deserialize(std::istream &in) {
    return Read(in, object) &&
           Read(in, timeDelayStart) &&
           Read(in, timeDelayEnd) &&
           Read(in, numberGroupOutput);
}

bool PhysicsState::Serialize(std::ostream &out) const {
    return Write(out, position) &&
           Write(out, orientation) &&
           Write(out, linearVelocity) &&
           Write(out, angularVelocity) &&
           Write(out, flags);
}

bool PhysicsState::Deserialize(std::istream &in) {
    return Read(in, position) &&
           Read(in, orientation) &&
           Read(in, linearVelocity) &&
           Read(in, angularVelocity) &&
           Read(in, flags);
}

bool PhysicsFrame::Serialize(std::ostream &out) const {
    return header.Serialize(out) &&
           Write(out, currentTime) &&
           Write(out, timeOfLastPSI) &&
           Write(out, timeOfNextPSI) &&
           envState.Serialize(out) &&
           WriteVector(out, objects);
}

bool PhysicsFrame::Deserialize(std::istream &in) {
    return header.Deserialize(in) &&
           Read(in, currentTime) &&
           Read(in, timeOfLastPSI) &&
           Read(in, timeOfNextPSI) &&
           envState.Deserialize(in) &&
           ReadVector(in, objects);
}

bool Sector::Serialize(std::ostream &out) const {
    return Write(out, id) &&
           Write(out, frameStart) &&
           Write(out, frameEnd) &&
           Write(out, startPosition) &&
           Write(out, endPosition) &&
           WriteVector(out, objects);
}

bool Sector::Deserialize(std::istream &in) {
    return Read(in, id) &&
           Read(in, frameStart) &&
           Read(in, frameEnd) &&
           Read(in, startPosition) &&
           Read(in, endPosition) &&
           ReadVector(in, objects);
}

bool InputState::Serialize(std::ostream &out) const {
    uint16_t keyStates = (keyUp << 0) | (keyDown << 1) | (keyLeft << 2) |
                         (keyRight << 3) | (keyShift << 4) | (keySpace << 5) |
                         (keyQ << 6) | (keyEsc << 7) | (keyEnter << 8);
    return Write(out, keyStates);
}

bool InputState::Deserialize(std::istream &in) {
    uint16_t keyStates;
    if (!Read(in, keyStates))
        return false;

    keyUp = (keyStates >> 0) & 1;
    keyDown = (keyStates >> 1) & 1;
    keyLeft = (keyStates >> 2) & 1;
    keyRight = (keyStates >> 3) & 1;
    keyShift = (keyStates >> 4) & 1;
    keySpace = (keyStates >> 5) & 1;
    keyQ = (keyStates >> 6) & 1;
    keyEsc = (keyStates >> 7) & 1;
    keyEnter = (keyStates >> 8) & 1;
    return true;
}

bool GameFrame::Serialize(std::ostream &out) const {
    return Write(out, deltaTime) &&
           inputState.Serialize(out);
}

bool GameFrame::Deserialize(std::istream &in) {
    return Read(in, deltaTime) &&
           inputState.Deserialize(in);
}

bool TASRecord::CompressData(const std::vector<uint8_t> &input, std::vector<uint8_t> &output) {
    uLongf compressedSize = compressBound(input.size());
    output.resize(compressedSize);
    if (compress(output.data(), &compressedSize, input.data(), input.size()) != Z_OK) {
        return false;
    }
    output.resize(compressedSize);
    return true;
}

bool TASRecord::DecompressData(const std::vector<uint8_t> &input, std::vector<uint8_t> &output) {
    uLongf decompressedSize = input.size() * 4; // Start with an estimated size
    output.resize(decompressedSize);
    while (true) {
        int res = uncompress(output.data(), &decompressedSize, input.data(), input.size());
        if (res == Z_OK) {
            output.resize(decompressedSize);
            return true;
        } else if (res == Z_BUF_ERROR) {
            decompressedSize *= 2;
            output.resize(decompressedSize);
        } else {
            return false;
        }
    }
}

void TASRecord::Load() {
    std::ifstream file(m_Path, std::ios::binary);
    if (!file.is_open())
        throw std::runtime_error("Failed to open file for reading");

    Clear();

    uint32_t magic, version;
    Serializable::Read(file, magic);
    if (magic != MAGIC_NUMBER)
        throw std::runtime_error("Invalid file format");

    Serializable::Read(file, version);
    if (version > VERSION)
        throw std::runtime_error("Unsupported file version");

    Serializable::Read(file, m_Flags);

    uint32_t checksum;
    Serializable::Read(file, checksum);

    size_t compressedSize;
    Serializable::Read(file, compressedSize);

    std::vector<uint8_t> compressedData(compressedSize);
    Serializable::ReadBytes(file, compressedData.data(), compressedSize);
    file.close();

    uint32_t checksumInFile = crc32(0, compressedData.data(), compressedData.size());
    if (checksum != checksumInFile) {
        throw std::runtime_error("Checksum mismatch");
    }

    std::vector<uint8_t> decompressedData;
    if (!DecompressData(compressedData, decompressedData)) {
        throw std::runtime_error("Failed to decompress data");
    }
    compressedData.clear();

    VectorInputStream memStream(decompressedData);

    Serializable::ReadString(memStream, m_MapName);

    size_t frameCount;
    Serializable::Read(memStream, frameCount);

    m_Frames.resize(frameCount);
    for (auto &frame : m_Frames) {
        if (!frame.Deserialize(memStream)) {
            throw std::runtime_error("Failed to deserialize a frame");
        }
    }

    size_t sectorCount;
    Serializable::Read(memStream, sectorCount);

    m_Sectors.resize(sectorCount);
    for (auto &sector : m_Sectors) {
        if (!sector.Deserialize(memStream)) {
            throw std::runtime_error("Failed to deserialize a sector");
        }
    }

    m_Loaded = true;
}

void TASRecord::Save() const {
    std::vector<uint8_t> data;
    VectorOutputStream memStream(data);

    Serializable::WriteString(memStream, m_MapName);

    size_t frameCount = m_Frames.size();
    Serializable::Write(memStream, frameCount);

    for (const auto &frame : m_Frames) {
        if (!frame.Serialize(memStream)) {
            throw std::runtime_error("Failed to serialize a frame");
        }
    }

    size_t sectorCount = m_Sectors.size();
    Serializable::Write(memStream, sectorCount);

    for (const auto &sector : m_Sectors) {
        if (!sector.Serialize(memStream)) {
            throw std::runtime_error("Failed to serialize a sector");
        }
    }

    std::vector<uint8_t> compressedData;
    if (!CompressData(data, compressedData)) {
        throw std::runtime_error("Failed to compress data");
    }

    std::ofstream file(m_Path, std::ios::binary);
    if (!file.is_open())
        throw std::runtime_error("Failed to open file for writing");

    Serializable::Write(file, MAGIC_NUMBER);
    Serializable::Write(file, VERSION);
    Serializable::Write(file, m_Flags);

    uint32_t checksum = crc32(0, compressedData.data(), compressedData.size());
    Serializable::Write(file, checksum);

    size_t compressedSize = compressedData.size();
    Serializable::Write(file, compressedSize);
    Serializable::WriteBytes(file, compressedData.data(), compressedSize);

    file.close();
}
