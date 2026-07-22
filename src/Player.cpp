#include "unmatched/Player.hpp"
#include "unmatched/GameExceptions.hpp"

#include <algorithm>
#include <utility>

namespace unmatched {

Player::Player(int id, std::string name, int age, HeroKind hero)
    : id_(id), name_(std::move(name)), age_(age), hero_(hero) {}

int Player::id() const {
    return id_;
}

const std::string& Player::name() const {
    return name_;
}

int Player::age() const {
    return age_;
}

HeroKind Player::hero() const {
    return hero_;
}

std::vector<Fighter>& Player::fighters() {
    return fighters_;
}

const std::vector<Fighter>& Player::fighters() const {
    return fighters_;
}

std::vector<Card>& Player::deck() {
    return deck_;
}

const std::vector<Card>& Player::deck() const {
    return deck_;
}

std::vector<Card>& Player::hand() {
    return hand_;
}

const std::vector<Card>& Player::hand() const {
    return hand_;
}

std::vector<Card>& Player::discardPile() {
    return discardPile_;
}

const std::vector<Card>& Player::discardPile() const {
    return discardPile_;
}

Fighter& Player::fighterById(const std::string& fighterId) {
    auto it = std::find_if(fighters_.begin(), fighters_.end(), [&](const Fighter& fighter) {
        return fighter.id() == fighterId;
    });
    if (it == fighters_.end()) {
        throw RuleViolation("Unknown fighter: " + fighterId);
    }
    return *it;
}

const Fighter& Player::fighterById(const std::string& fighterId) const {
    auto it = std::find_if(fighters_.begin(), fighters_.end(), [&](const Fighter& fighter) {
        return fighter.id() == fighterId;
    });
    if (it == fighters_.end()) {
        throw RuleViolation("Unknown fighter: " + fighterId);
    }
    return *it;
}

Fighter& Player::heroFighter() {
    auto it = std::find_if(fighters_.begin(), fighters_.end(), [](const Fighter& fighter) {
        return fighter.isHero();
    });
    if (it == fighters_.end()) {
        throw RuleViolation("Player has no hero.");
    }
    return *it;
}

const Fighter& Player::heroFighter() const {
    auto it = std::find_if(fighters_.begin(), fighters_.end(), [](const Fighter& fighter) {
        return fighter.isHero();
    });
    if (it == fighters_.end()) {
        throw RuleViolation("Player has no hero.");
    }
    return *it;
}

std::vector<Fighter*> Player::aliveFighters() {
    std::vector<Fighter*> result;
    for (auto& fighter : fighters_) {
        if (!fighter.defeated()) {
            result.push_back(&fighter);
        }
    }
    return result;
}

std::vector<const Fighter*> Player::aliveFighters() const {
    std::vector<const Fighter*> result;
    for (const auto& fighter : fighters_) {
        if (!fighter.defeated()) {
            result.push_back(&fighter);
        }
    }
    return result;
}

Card Player::removeCardFromHand(int index) {
    if (index < 0 || index >= static_cast<int>(hand_.size())) {
        throw RuleViolation("Invalid hand card index.");
    }
    Card card = hand_.at(static_cast<std::size_t>(index));
    hand_.erase(hand_.begin() + index);
    return card;
}

void Player::addToDiscard(Card card) {
    discardPile_.push_back(std::move(card));
}

void Player::addToHand(Card card) {
    hand_.push_back(std::move(card));
}

bool Player::hasLivingCharacter(Character character) const {
    if (character == Character::Any) {
        return !aliveFighters().empty();
    }
    return std::any_of(fighters_.begin(), fighters_.end(), [&](const Fighter& fighter) {
        return !fighter.defeated() && fighter.cardOwner() == character;
    });
}

}  // namespace unmatched
