#pragma once
#include <string>

namespace unmatched {

class GameEntity {
protected:
    std::string id_;
    std::string name_;
    bool isActive_;

public:
    GameEntity(const std::string& id, const std::string& name)
        : id_(id), name_(name), isActive_(true) {}
    
    virtual ~GameEntity() = default;

    std::string getId() const { return id_; }
    std::string getName() const { return name_; }
    bool isActive() const { return isActive_; }
    
    void setId(const std::string& id) { id_ = id; }
    void setName(const std::string& name) { name_ = name; }
    void setActive(bool active) { isActive_ = active; }
};

} // namespace unmatched
