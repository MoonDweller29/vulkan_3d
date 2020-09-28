#include "app/config/config.h"
#include <vector>
#include <string>
#include <iostream>

struct Person {
    std::string name;
    int age;

    JSON_MAPPINGS(
            {age, "age"},
            {name, "name"}
    )
};

struct Team {
    std::string name;
    std::vector<Person> members;
    float score;
    Person captain;

    JSON_MAPPINGS(
            {members, "members"},
            {name, "name"},
            {score, "score"},
            {captain, "captain"}
    )
};


void test_config() {
    Person dude = {"Dude", 25};
    Person boi = {"Boi"};
    Person mysterious;

    Team team_a {
        "Team A",
        {
            {"Dude", 25},
            {"Boi"},
            {}
        },
        22.8,
        {"Captain", 99}
    };

    json dump = team_a;
    std::cout << dump.dump(4) << std::endl;

    Team team_b = dump.get<Team>();
    json dump_2 = team_b;
    std::cout << dump_2.dump(4) << std::endl;
}