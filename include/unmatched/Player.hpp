#pragma once

#include "unmatched/Card.hpp"
#include "unmatched/Fighter.hpp"
#include "unmatched/GameExceptions.hpp"

#include <optional>
#include <string>
#include <vector>

namespace unmatched {

class Player {
public:
    Player(int id, std::string name, int age, HeroKind hero);

    int id() const;
    const std::string& name() const;
    int age() const;
    HeroKind hero() const;
    std::vector<Fighter>& fighters();
    const std::vector<Fighter>& fighters() const;
    std::vector<Card>& deck();
    const std::vector<Card>& deck() const;
    std::vector<Card>& hand();
    const std::vector<Card>& hand() const;
    std::vector<Card>& discardPile();
    const std::vector<Card>& discardPile() const;

    Fighter& fighterById(const std::string& fighterId);
    const Fighter& fighterById(const std::string& fighterId) const;
    Fighter& heroFighter();
    const Fighter& heroFighter() const;
    std::vector<Fighter*> aliveFighters();
    std::vector<const Fighter*> aliveFighters() const;

    Card removeCardFromHand(int index);
    void addToDiscard(Card card);
    void addToHand(Card card);
    bool hasLivingCharacter(Character character) const;

private:
    int id_;
    std::string name_;
    int age_;
    HeroKind hero_;
    std::vector<Fighter> fighters_;
    std::vector<Card> deck_;
    std::vector<Card> hand_;
    std::vector<Card> discardPile_;
};

}  // namespace unmatched