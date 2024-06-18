#if 0
#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>
#include <vector>
#include <map>
#include <string>
#include <regex>

using json = nlohmann::json;

void generateStruct(const json& schema, std::ofstream& outfile);
void generateField(const json& field, std::ofstream& outfile);
void generateEnum(const json& enumDef, std::ofstream& outfile);

bool validateJsonSchema(const json& schema) {
    if (!schema.contains("name") || !schema.contains("fields")) {
        std::cerr << "Error: JSON schema is missing required fields.\n";
        return false;
    }

    const auto& fields = schema["fields"];
    for (const auto& field : fields) {
        if (!field.contains("name") || !field.contains("type")) {
            std::cerr << "Error: Each field must have 'name' and 'type'.\n";
            return false;
        }
        std::string type = field["type"];
        if (type != "int" && type != "string" && type != "date" && type != "address" &&
            type != "vector" && type != "map" && type != "enum" && type != "struct") {
            std::cerr << "Error: Unsupported type '" << type << "' for field '" << field["name"] << "'.\n";
            return false;
        }

        // Validate nested schema for vector type
        if (type == "vector" && !field.contains("item_type")) {
            std::cerr << "Error: Vector field '" << field["name"] << "' must specify 'item_type'.\n";
            return false;
        }

        // Validate nested schema for map type
        if (type == "map" && (!field.contains("key_type") || !field.contains("value_type"))) {
            std::cerr << "Error: Map field '" << field["name"] << "' must specify 'key_type' and 'value_type'.\n";
            return false;
        }

        // Validate nested schema for struct type
        if (type == "struct" && !field.contains("fields")) {
            std::cerr << "Error: Struct field '" << field["name"] << "' must specify 'fields'.\n";
            return false;
        }

        // Validate enum type
        if (type == "enum" && (!field.contains("enum_name") || !field.contains("values"))) {
            std::cerr << "Error: Enum field '" << field["name"] << "' must specify 'enum_name' and 'values'.\n";
            return false;
        }
    }

    return true;
}

void generateEnum(const json& enumDef, std::ofstream& outfile) {
    std::string enumName = enumDef["enum_name"];
    outfile << "enum class " << enumName << " {\n";
    const auto& values = enumDef["values"];
    for (size_t i = 0; i < values.size(); ++i) {
        outfile << "    " << values[i];
        if (i < values.size() - 1) {
            outfile << ",";
        }
        outfile << "\n";
    }
    outfile << "};\n\n";
}

void generateField(const json& field, std::ofstream& outfile) {
    std::string type = field["type"];
    std::string name = field["name"];

    if (type == "int" || type == "string") {
        outfile << "    " << type << " " << name << ";\n";
    } else if (type == "date") {
        outfile << "    Date " << name << ";\n";
    } else if (type == "address") {
        outfile << "    Address " << name << ";\n";
    } else if (type == "vector") {
        outfile << "    std::vector<" << field["item_type"] << "> " << name << ";\n";
    } else if (type == "map") {
        outfile << "    std::map<" << field["key_type"] << ", " << field["value_type"] << "> " << name << ";\n";
    } else if (type == "enum") {
        outfile << "    " << field["enum_name"] << " " << name << ";\n";
    } else if (type == "struct") {
        generateStruct(field, outfile);
        outfile << "    " << field["name"] << " " << name << ";\n";
    }
}

void generateValidationFunction(const json& schema, std::ofstream& outfile) {
    const json& fields = schema["fields"];

    outfile << "    bool validate() const {\n";

    for (const auto& field : fields) {
        std::string name = field["name"];
        std::string type = field["type"];

        if (type == "int") {
            if (field.contains("min")) {
                outfile << "        if (" << name << " < " << field["min"] << ") return false;\n";
            }
            if (field.contains("max")) {
                outfile << "        if (" << name << " > " << field["max"] << ") return false;\n";
            }
        } else if (type == "string") {
            if (field.contains("max_length")) {
                outfile << "        if (" << name << ".length() > " << field["max_length"] << ") return false;\n";
            }
            if (field.contains("regex")) {
                outfile << "        if (!std::regex_match(" << name << ", std::regex(\"" << field["regex"] << "\"))) return false;\n";
            }
        } else if (type == "vector") {
            outfile << "        for (const auto& item : " << name << ") {\n";
            outfile << "            if (!item.validate()) return false;\n";
            outfile << "        }\n";
        } else if (type == "map") {
            if (field["value_type"] == "int") {
                if (field.contains("value_min")) {
                    outfile << "        for (const auto& [key, value] : " << name << ") {\n";
                    outfile << "            if (value < " << field["value_min"] << ") return false;\n";
                    outfile << "        }\n";
                }
                if (field.contains("value_max")) {
                    outfile << "        for (const auto& [key, value] : " << name << ") {\n";
                    outfile << "            if (value > " << field["value_max"] << ") return false;\n";
                    outfile << "        }\n";
                }
            } else {
                outfile << "        for (const auto& [key, value] : " << name << ") {\n";
                outfile << "            if (!value.validate()) return false;\n";
                outfile << "        }\n";
            }
        }
    }

    outfile << "        return true;\n";
    outfile << "    }\n";
}

void generateStruct(const json& schema, std::ofstream& outfile) {
    outfile << "struct " << schema["name"] << " {\n";
    const json& fields = schema["fields"];
    for (const auto& field : fields) {
        generateField(field, outfile);
    }

    // Generate validation function for the struct
    generateValidationFunction(schema, outfile);

    outfile << "};\n\n";
}

void generateCustomTypes(const json& schema, std::ofstream& outfile) {
    if (schema.contains("definitions")) {
        const json& definitions = schema["definitions"];
        for (const auto& definition : definitions) {
            generateStruct(definition, outfile);
        }
    }

    if (schema.contains("fields")) {
        const json& fields = schema["fields"];
        for (const auto& field : fields) {
            if (field.contains("type") && field["type"] == "enum") {
                generateEnum(field, outfile);
            }
        }
    }
}

void generateCppCode(const json& schema) {
    if (!validateJsonSchema(schema)) {
        return;
    }

    std::ofstream outfile("generated_code.cpp");

    // Include necessary headers
    outfile << "#include <iostream>\n";
    outfile << "#include <vector>\n";
    outfile << "#include <map>\n";
    outfile << "#include <string>\n";
    outfile << "#include <regex>\n\n";

    // Generate custom types from definitions
    generateCustomTypes(schema, outfile);

    // Generate main struct/class
    generateStruct(schema, outfile);

    // Example usage in main function
    outfile << "int main() {\n"
               "    " << schema["name"] << " obj;\n"
               "    // Initialize and use obj\n"
               "    if (!obj.validate()) {\n"
               "        std::cerr << \"Validation failed!\" << std::endl;\n"
               "        return 1;\n"
               "    }\n"
               "    return 0;\n"
               "}\n";

    outfile.close();
    std::cout << "C++ code generation complete.\n";
}

int main() {
    // Example JSON schema (replace with actual JSON parsing)
    json schema = R"(
    {
        "name": "Person",
        "fields": [
            {
                "name": "id",
                "type": "int",
                "min": 1,
                "max": 1000
            },
            {
                "name": "name",
                "type": "string",
                "max_length": 50,
                "regex": "^[A-Za-z ]+$"
            },
            {
                "name": "birthdate",
                "type": "date"
            },
            {
                "name": "address",
                "type": "address"
            },
            {
                "name": "gender",
                "type": "enum",
                "enum_name": "Gender",
                "values": ["Male", "Female", "Other"]
            },
            {
                "name": "children",
                "type": "vector",
                "item_type": "Child",
                "fields": [
                    {
                        "name": "name",
                        "type": "string",
                        "max_length": 50,
                        "regex": "^[A-Za-z ]+$"
                    },
                    {
                        "name": "birthdate",
                        "type": "date"
                    }
                ]
            },
            {
                "name": "phone_book",
                "type": "map",
                "key_type": "string",
                "value_type": "string"
            },
            {
                "name": "grades",
                "type": "map",
                "key_type": "string",
                "value_type": "int",
                "value_min": 0,
                "value_max": 100
            }
        ],
        "definitions": [
            {
                "name": "Date",
                "fields": [
                    { "name": "day", "type": "int", "min": 1, "max": 31 },
                    { "name": "month", "type": "int", "min": 1, "max": 12 },
                    { "name": "year", "type": "int", "min": 1900, "max": 2100 }
                ]
            },
            {
                "name": "Address",
                "fields": [
                    { "name": "street", "type": "string", "max_length": 100 },
                    { "name": "city", "type": "string", "max_length": 50 },
                    { "name": "zip", "type": "string", "max_length": 10, "regex": "^[0-9]{5}(-[0-9]{4})?$" }
                ]
            }
        ]
    }
    )"_json;

    generateCppCode(schema);

    return 0;
}

#endif


#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>
#include <vector>
#include <map>
#include <string>
#include <regex>

using json = nlohmann::json;

void generateStruct(const std::string& typeName, const json& schema, std::ofstream& outfile);
void generateField(const json& field, std::ofstream& outfile);
void generateEnum(const json& enumDef, std::ofstream& outfile);

bool validateJsonSchema(const json& schema) {
    if (!schema.contains("types")) {
        std::cerr << "Error: JSON schema is missing 'types' definition.\n";
        return false;
    }

    const auto& types = schema["types"];
    for (auto it = types.begin(); it != types.end(); ++it) {
        const std::string& typeName = it.key();
        const auto& typeDef = it.value();

        if (!typeDef.contains("fields")) {
            std::cerr << "Error: Type '" << typeName << "' is missing 'fields'.\n";
            return false;
        }

        const auto& fields = typeDef["fields"];
        for (const auto& field : fields) {
            if (!field.contains("name") || !field.contains("type")) {
                std::cerr << "Error: Each field in type '" << typeName << "' must have 'name' and 'type'.\n";
                return false;
            }
            std::string type = field["type"];
            if (type != "int" && type != "string" && type != "date" && type != "address" &&
                type != "vector" && type != "map" && type != "enum" && type != typeName) {
                std::cerr << "Error: Unsupported type '" << type << "' for field '" << field["name"] << "' in type '" << typeName << "'.\n";
                return false;
            }

            // Validate nested schema for vector type
            if (type == "vector" && !field.contains("item_type")) {
                std::cerr << "Error: Vector field '" << field["name"] << "' in type '" << typeName << "' must specify 'item_type'.\n";
                return false;
            }

            // Validate nested schema for map type
            if (type == "map" && (!field.contains("key_type") || !field.contains("value_type"))) {
                std::cerr << "Error: Map field '" << field["name"] << "' in type '" << typeName << "' must specify 'key_type' and 'value_type'.\n";
                return false;
            }

            // Validate enum type
            if (type == "enum" && (!field.contains("enum_name") || !field.contains("values"))) {
                std::cerr << "Error: Enum field '" << field["name"] << "' in type '" << typeName << "' must specify 'enum_name' and 'values'.\n";
                return false;
            }
        }
    }

    return true;
}

void generateEnum(const json& enumDef, std::ofstream& outfile) {
    std::string enumName = enumDef["enum_name"];
    outfile << "enum class " << enumName << " {\n";
    const auto& values = enumDef["values"];
    for (size_t i = 0; i < values.size(); ++i) {
        outfile << "    " << values[i];
        if (i < values.size() - 1) {
            outfile << ",";
        }
        outfile << "\n";
    }
    outfile << "};\n\n";
}

void generateField(const json& field, std::ofstream& outfile) {
    std::string type = field["type"];
    std::string name = field["name"];

    if (type == "int" || type == "string") {
        outfile << "    " << type << " " << name << ";\n";
    } else if (type == "date") {
        outfile << "    Date " << name << ";\n";
    } else if (type == "address") {
        outfile << "    Address " << name << ";\n";
    } else if (type == "vector") {
        outfile << "    std::vector<" << field["item_type"] << "> " << name << ";\n";
    } else if (type == "map") {
        outfile << "    std::map<" << field["key_type"] << ", " << field["value_type"] << "> " << name << ";\n";
    } else if (type == "enum") {
        outfile << "    " << field["enum_name"] << " " << name << ";\n";
    } else {
        // Handle user-defined types
        outfile << "    " << type << " " << name << ";\n";
    }
}

void generateStruct(const std::string& typeName, const json& schema, std::ofstream& outfile) {
    const auto& typeDef = schema["types"][typeName];

    outfile << "struct " << typeName << " {\n";

    const auto& fields = typeDef["fields"];
    for (const auto& field : fields) {
        generateField(field, outfile);
    }

    outfile << "};\n\n";
}

void generateCppCode(const json& schema) {
    if (!validateJsonSchema(schema)) {
        //return;
    }

    std::ofstream outfile("generated_code.cpp");

    // Include necessary headers
    outfile << "#include <iostream>\n";
    outfile << "#include <vector>\n";
    outfile << "#include <map>\n";
    outfile << "#include <string>\n";
    outfile << "#include <regex>\n\n";

    // Generate enums and custom types
    const auto& types = schema["types"];
    for (auto it = types.begin(); it != types.end(); ++it) {
        const std::string& typeName = it.key();
        const auto& typeDef = it.value();

        if (typeDef.contains("enum_name")) {
            generateEnum(typeDef, outfile);
        } else {
            generateStruct(typeName, schema, outfile);
        }
    }

    // Example usage in main function
    outfile << "int main() {\n"
               "    Person p;\n"
               "    // Initialize and use p\n"
               "    return 0;\n"
               "}\n";

    outfile.close();
    std::cout << "C++ code generation complete.\n";
}

int main() {
    // Example JSON schema (replace with actual JSON parsing)
    json schema = R"(
    {
        "types": {
            "AnotherData": {
                "fields": [
                    {
                        "name": "field1",
                        "type": "string"
                    },
                    {
                        "name": "field2",
                        "type": "int",
                        "min": 0,
                        "max": 100
                    }
                ]
            },
            "OtherData": {
                "fields": [
                    {
                        "name": "field1",
                        "type": "string"
                    },
                    {
                        "name": "field2",
                        "type": "int",
                        "min": 0,
                        "max": 100
                    },
                    {
                        "name": "anotherData",
                        "type": "AnotherData"
                    }
                ]
            },
            "Person": {
                "fields": [
                    {
                        "name": "id",
                        "type": "int",
                        "min": 1,
                        "max": 1000
                    },
                    {
                        "name": "name",
                        "type": "string",
                        "max_length": 50,
                        "regex": "^[A-Za-z ]+$"
                    },
                    {
                        "name": "birthdate",
                        "type": "date"
                    },
                    {
                        "name": "address",
                        "type": "address"
                    },
                    {
                        "name": "otherData",
                        "type": "OtherData"
                    }
                ]
            },
            "Date": {
                "fields": [
                    { "name": "day", "type": "int", "min": 1, "max": 31 },
                    { "name": "month", "type": "int", "min": 1, "max": 12 },
                    { "name": "year", "type": "int", "min": 1900, "max": 2100 }
                ]
            },
            "Address": {
                "fields": [
                    { "name": "street", "type": "string", "max_length": 100 },
                    { "name": "city", "type": "string", "max_length": 50 },
                    { "name": "zip", "type": "string", "max_length": 10, "regex": "^[0-9]{5}(-[0-9]{4})?$" }
                ]
            }
        }
    }
    )"_json;

    generateCppCode(schema);
}

