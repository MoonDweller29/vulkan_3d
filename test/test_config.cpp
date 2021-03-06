#include "magma/app/config/JSON.h"
#include <vector>
#include <string>
#include <iostream>
#include <cstdio>

enum class Gender {
    UNSPECIFIED,
    MALE,
    FEMALE
};

JSON_ENUM_MAPPING(Gender,
    {Gender::MALE, "male"},
    {Gender::FEMALE, "w*man"}
)


struct Person {
    std::string name = "Unknown";
    int age = 18;
    Gender gender = Gender::UNSPECIFIED;

    JSON_MAPPINGS(
            {age, "age"},
            {name, "name"},
            {gender, "gender"}
    )
};

struct Team {
    std::string name = "Unnamed";
    std::vector<Person> members;
    float score;
    Person captain;

private:
    JSON_MAPPINGS(
            {members, "members"},
            {name, "name"},
            {score, "score"},
            {captain, "captain"}
    )

};

namespace hidden {
    struct Sneaky {
        int get_x() const {
            return x;
        }

    private:
        int x = 42;

        JSON_MAPPINGS(
                { x, "x" }
        )
    };
}

void test_config() {
    Team team_a {
        "Team A",
        {
            {"Dude", 25, Gender::FEMALE},
            {"Boi"},
            {}
        },
        22.8,
        {"Captain", 99, Gender::MALE}
    };

    JSON dump = team_a;
    std::cout << dump.dump(4) << std::endl;

    Team team_b;
    team_b = dump.get<Team>();
    JSON dump_2 = team_b;
    std::cout << dump_2.dump(4) << std::endl;

    hidden::Sneaky sneaky;
    JSON sneaky_json = sneaky;
    std::cout << sneaky_json.dump(4) << std::endl;
    sneaky_json["x"] = 12;
    hidden::Sneaky another_sneaky = sneaky_json.get<hidden::Sneaky>();
    std::cout << another_sneaky.get_x() << std::endl;

    std::string test_person_filename = "test_person.json";
    std::remove(test_person_filename.c_str());
    Person test_person = json::load<Person>(test_person_filename); //must log error if file not exist
    test_person.age = 98;
    test_person.gender = Gender::FEMALE;
    test_person.name = "JSON_PERSON";

    json::save(test_person_filename, test_person);
    test_person.age = 2;
    test_person = json::load<Person>(test_person_filename); //should be ok now
    JSON test_person_json = test_person;
    std::cout << test_person_json.dump(4) << std::endl;

}
