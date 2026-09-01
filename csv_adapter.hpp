#pragma once

#include "value.hpp"
#include "rapidcsv.h"

#include <algorithm>
#include <any>
#include <array>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace csvmap {

class CsvAdapterError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class SchemaError : public CsvAdapterError {
public:
    using CsvAdapterError::CsvAdapterError;
};

class SchemaMismatchError : public CsvAdapterError {
public:
    using CsvAdapterError::CsvAdapterError;
};

class CellParseError : public CsvAdapterError {
public:
    using CsvAdapterError::CsvAdapterError;
};

class FactoryError : public CsvAdapterError {
public:
    using CsvAdapterError::CsvAdapterError;
};

enum class FieldType { Bool, Int, Double, Char, String };

inline std::string fieldTypeName(FieldType type) {
    switch (type) {
        case FieldType::Bool:   return "bool";
        case FieldType::Int:    return "int";
        case FieldType::Double: return "double";
        case FieldType::Char:   return "char";
        case FieldType::String: return "string";
    }
    return "unknown";
}

enum class Delimiter { Comma, Semicolon, Tab, Pipe, Auto };

inline char delimiterChar(Delimiter d) {
    switch (d) {
        case Delimiter::Comma:     return ',';
        case Delimiter::Semicolon: return ';';
        case Delimiter::Tab:       return '\t';
        case Delimiter::Pipe:      return '|';
        case Delimiter::Auto:      break;
    }
    throw CsvAdapterError("delimiter 'Auto' has no fixed character");
}

inline std::string toLower(std::string s) {
    for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

inline std::string trim(const std::string& s) {
    const std::size_t first = s.find_first_not_of(" \t\r\n\f\v");
    if (first == std::string::npos) return {};
    const std::size_t last = s.find_last_not_of(" \t\r\n\f\v");
    return s.substr(first, last - first + 1);
}

inline char delimiterFromToken(const std::string& token, const std::string& context) {
    const std::string t = trim(token);
    if (t == "," || t == "comma")     return ',';
    if (t == ";" || t == "semicolon") return ';';
    if (t == "\t" || t == "\\t" || t == "tab") return '\t';
    if (t == "|" || t == "pipe")      return '|';
    if (t.size() == 1) return t[0];
    throw SchemaError("invalid delimiter '" + token + "' in " + context +
                      " (expected comma, semicolon, tab, pipe, or a single character)");
}

struct CellContext {
    std::string file;
    std::string fieldName;
    std::size_t dataRow = 0;
};

struct Field {
    std::string name;
    FieldType type = FieldType::String;
    bool optional = false;
    bool hasDefault = false;
    value defaultValue;

    bool required() const { return !optional && !hasDefault; }
};

inline FieldType parseFieldType(const std::string& token, const std::string& context,
                                const std::string& fieldName) {
    const std::string t = toLower(trim(token));
    if (t == "bool")   return FieldType::Bool;
    if (t == "int")    return FieldType::Int;
    if (t == "double") return FieldType::Double;
    if (t == "char")   return FieldType::Char;
    if (t == "string") return FieldType::String;
    throw SchemaError("unknown type '" + token + "' for field '" + fieldName + "' in " + context +
                      " (supported: bool, int, double, char, string)");
}

inline value parseCell(const std::string& raw, const Field& field, const CellContext& ctx) {
    const std::string text = trim(raw);

    if (text.empty()) {
        if (field.hasDefault) return field.defaultValue;
        if (field.optional)   return value();
        throw CellParseError(ctx.file + ": data row " + std::to_string(ctx.dataRow + 1) +
                             ": missing value for required field '" + field.name + "'");
    }

    auto fail = [&field, &ctx](const std::string& why) {
        return CellParseError(ctx.file + ": data row " + std::to_string(ctx.dataRow + 1) +
                              ", column '" + field.name + "': " + why);
    };

    switch (field.type) {
        case FieldType::String:
            return value(text);

        case FieldType::Char:
            if (text.size() != 1)
                throw fail("expected a single character, got '" + text + "'");
            return value(text[0]);

        case FieldType::Bool: {
            const std::string s = toLower(text);
            if (s == "true" || s == "yes" || s == "y" || s == "t" || s == "1") return value(true);
            if (s == "false" || s == "no" || s == "n" || s == "f" || s == "0") return value(false);
            throw fail("cannot parse '" + text + "' as bool (expected true/false, yes/no, y/n, t/f, 1/0)");
        }

        case FieldType::Int:
            try {
                std::size_t pos = 0;
                const int i = std::stoi(text, &pos);
                if (pos != text.size())
                    throw fail("cannot parse '" + text + "' as int (trailing characters '" +
                               text.substr(pos) + "')");
                return value(i);
            } catch (const CellParseError&) {
                throw;
            } catch (const std::out_of_range&) {
                throw fail("value '" + text + "' out of range for int");
            } catch (const std::exception&) {
                throw fail("cannot parse '" + text + "' as int");
            }

        case FieldType::Double:
            try {
                std::size_t pos = 0;
                const double d = std::stod(text, &pos);
                if (pos != text.size())
                    throw fail("cannot parse '" + text + "' as double (trailing characters '" +
                               text.substr(pos) + "')");
                return value(d);
            } catch (const CellParseError&) {
                throw;
            } catch (const std::out_of_range&) {
                throw fail("value '" + text + "' out of range for double");
            } catch (const std::exception&) {
                throw fail("cannot parse '" + text + "' as double");
            }
    }
    throw fail("unhandled field type '" + fieldTypeName(field.type) + "'");
}

struct Record {
    std::string className;
    std::string sourceFile;
    std::size_t dataRow = 0;
    std::vector<std::pair<std::string, value>> fields;

    const value& at(const std::string& fieldName) const {
        for (const auto& entry : fields)
            if (toLower(entry.first) == toLower(fieldName)) return entry.second;
        throw CsvAdapterError("record of class '" + className + "' has no field '" + fieldName + "'");
    }

    const value& at(std::size_t index) const {
        if (index >= fields.size())
            throw CsvAdapterError("record field index out of range: " + std::to_string(index));
        return fields[index].second;
    }
};

struct Schema {
    std::string className;
    std::string source;
    std::vector<Field> fields;
    std::optional<char> defaultDelimiter;

    const Field* findField(const std::string& fieldName) const {
        for (const Field& f : fields)
            if (toLower(f.name) == toLower(fieldName)) return &f;
        return nullptr;
    }
};

inline Schema parseSchemaFile(const std::filesystem::path& path) {
    std::ifstream in(path);
    if (!in)
        throw SchemaError("cannot open schema file: " + path.string());

    Schema schema;
    schema.source = path.string();
    schema.className = path.stem().string();

    bool fieldsStarted = false;
    std::string raw;
    std::size_t lineNo = 0;

    while (std::getline(in, raw)) {
        ++lineNo;
        if (!raw.empty() && raw.back() == '\r') raw.pop_back();

        std::string text = trim(raw);
        if (text.empty()) continue;
        if (text.rfind("//", 0) == 0) continue;
        if (text[0] == '#') continue;
        if (text == "---") continue;

        const std::string context = path.string() + ":" + std::to_string(lineNo);

        if (text.rfind("class", 0) == 0 &&
            (text.size() == 5 || std::isspace(static_cast<unsigned char>(text[5])))) {
            if (fieldsStarted)
                throw SchemaError(context + ": 'class' directive must appear before any field");
            const std::string name = trim(text.substr(5));
            if (name.empty() || name.find_first_of(" \t") != std::string::npos)
                throw SchemaError(context + ": expected 'class <ClassName>'");
            schema.className = name;
            continue;
        }

        if (text.rfind("delimiter", 0) == 0 &&
            (text.size() == 9 || std::isspace(static_cast<unsigned char>(text[9])))) {
            if (fieldsStarted)
                throw SchemaError(context + ": 'delimiter' directive must appear before any field");
            schema.defaultDelimiter = delimiterFromToken(trim(text.substr(9)), context);
            continue;
        }

        const std::size_t colon = text.find(':');
        if (colon == std::string::npos)
            throw SchemaError(context + ": expected 'name : type' field definition, got '" + text + "'");

        const std::string name = trim(text.substr(0, colon));
        std::string spec = trim(text.substr(colon + 1));
        if (name.empty())
            throw SchemaError(context + ": empty field name");
        if (name.find_first_of(" \t") != std::string::npos)
            throw SchemaError(context + ": field name '" + name + "' must not contain whitespace");

        Field field;
        field.name = name;

        const std::size_t eq = spec.find('=');
        if (eq != std::string::npos) {
            field.hasDefault = true;
            const std::string defaultValue = trim(spec.substr(eq + 1));
            spec = trim(spec.substr(0, eq));
            if (defaultValue.empty())
                throw SchemaError(context + ": empty default value for field '" + name + "'");
            CellContext ctx{path.string(), field.name, 0};
            field.defaultValue = parseCell(defaultValue, field, ctx);
        }

        if (!spec.empty() && spec.back() == '?') {
            field.optional = true;
            spec.pop_back();
            spec = trim(spec);
        }

        if (spec.empty())
            throw SchemaError(context + ": missing type for field '" + name + "'");
        field.type = parseFieldType(spec, context, name);

        if (schema.findField(field.name))
            throw SchemaError(context + ": duplicate field '" + field.name + "'");

        schema.fields.push_back(std::move(field));
        fieldsStarted = true;
    }

    if (schema.fields.empty())
        throw SchemaError("schema file '" + path.string() + "' defines no fields");

    return schema;
}

class SchemaBook {
public:
    std::size_t scanFolder(const std::filesystem::path& folder, bool recursive = false) {
        std::error_code ec;
        if (!std::filesystem::is_directory(folder, ec))
            throw SchemaError("cannot scan schema folder (not a directory): " + folder.string());

        std::vector<std::filesystem::path> files;
        const auto collect = [&](const std::filesystem::directory_entry& entry) {
            if (entry.is_regular_file() && entry.path().extension() == ".schema")
                files.push_back(entry.path());
        };
        if (recursive) {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(folder))
                collect(entry);
        } else {
            for (const auto& entry : std::filesystem::directory_iterator(folder))
                collect(entry);
        }

        std::sort(files.begin(), files.end());
        for (const auto& file : files)
            add(parseSchemaFile(file));
        return files.size();
    }

    void add(Schema schema) {
        const std::string key = toLower(schema.className);
        const auto it = schemas_.find(key);
        if (it != schemas_.end())
            throw SchemaError("duplicate class definition '" + schema.className + "' found in '" +
                              it->second.source + "' and '" + schema.source + "'");
        schemas_.emplace(std::move(key), std::move(schema));
    }

    bool contains(const std::string& className) const {
        return schemas_.find(toLower(className)) != schemas_.end();
    }

    const Schema& get(const std::string& className) const {
        const auto it = schemas_.find(toLower(className));
        if (it == schemas_.end())
            throw SchemaError("no schema registered for class '" + className + "'");
        return it->second;
    }

    std::vector<std::string> classNames() const {
        std::vector<std::string> names;
        names.reserve(schemas_.size());
        for (const auto& [key, schema] : schemas_) names.push_back(schema.className);
        std::sort(names.begin(), names.end());
        return names;
    }

    std::size_t size() const { return schemas_.size(); }

private:
    std::unordered_map<std::string, Schema> schemas_;
};

inline char detectDelimiter(const std::filesystem::path& file) {
    std::ifstream in(file);
    if (!in)
        throw CsvAdapterError("cannot open CSV file: " + file.string());

    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (!line.empty()) break;
    }
    if (line.empty()) return ',';

    static constexpr char kCandidates[] = {',', ';', '\t', '|'};
    std::array<std::size_t, 4> counts{};
    bool inQuotes = false;
    for (const char c : line) {
        if (c == '"') {
            inQuotes = !inQuotes;
        } else if (!inQuotes) {
            for (std::size_t i = 0; i < counts.size(); ++i)
                if (c == kCandidates[i]) ++counts[i];
        }
    }

    std::size_t best = 0;
    for (std::size_t i = 1; i < counts.size(); ++i)
        if (counts[i] > counts[best]) best = i;
    return counts[best] > 0 ? kCandidates[best] : ',';
}

inline std::unique_ptr<rapidcsv::Document> loadDocument(const std::filesystem::path& file,
                                                        char separator,
                                                        bool hasHeader,
                                                        bool trimCells) {
    {
        std::ifstream probe(file);
        if (!probe.good())
            throw CsvAdapterError("cannot open CSV file: " + file.string());
    }

    const rapidcsv::LabelParams labels(hasHeader ? 0 : -1, -1);
    const rapidcsv::SeparatorParams separators(separator, trimCells);
    try {
        return std::make_unique<rapidcsv::Document>(file.string(), labels, separators);
    } catch (const std::exception& e) {
        throw CsvAdapterError("failed to load CSV file '" + file.string() + "': " + e.what());
    }
}

class ObjectFactory {
public:
    using Maker = std::function<std::any(const Record&)>;

    void add(const std::string& className, Maker maker) {
        makers_[toLower(className)] = std::move(maker);
    }

    template <typename T, typename Fn>
    void registerClass(const std::string& className, Fn&& maker) {
        add(className, [fn = std::forward<Fn>(maker)](const Record& record) -> std::any {
            return fn(record);
        });
    }

    bool contains(const std::string& className) const {
        return makers_.find(toLower(className)) != makers_.end();
    }

    template <typename T>
    std::shared_ptr<T> create(const std::string& className, const Record& record) const {
        const auto it = makers_.find(toLower(className));
        if (it == makers_.end())
            throw FactoryError("no factory registered for class '" + className + "'");
        std::any product = it->second(record);
        const auto typed = std::any_cast<std::shared_ptr<T>>(&product);
        if (!typed)
            throw FactoryError("factory for class '" + className +
                               "' produced a value of an incompatible type");
        return *typed;
    }

private:
    std::unordered_map<std::string, Maker> makers_;
};

struct LoadOptions {
    Delimiter delimiter = Delimiter::Auto;
    bool hasHeader = true;
    bool trimCells = true;
    bool ignoreExtraColumns = true;
    bool skipEmptyRows = true;
};

class CsvAdapter {
public:
    CsvAdapter(SchemaBook schemas, ObjectFactory factory = {})
        : schemas_(std::move(schemas)), factory_(std::move(factory)) {}

    const SchemaBook& schemas() const { return schemas_; }
    ObjectFactory& factory() { return factory_; }
    const ObjectFactory& factory() const { return factory_; }

    void setWarningHandler(std::function<void(const std::string&)> handler) {
        warn_ = std::move(handler);
    }

    std::vector<Record> mapFile(const std::filesystem::path& file,
                                const std::string& className,
                                const LoadOptions& options = {}) const {
        const Schema& schema = schemas_.get(className);

        char separator = 0;
        if (options.delimiter != Delimiter::Auto)
            separator = delimiterChar(options.delimiter);
        else if (schema.defaultDelimiter)
            separator = *schema.defaultDelimiter;
        else
            separator = detectDelimiter(file);

        auto doc = loadDocument(file, separator, options.hasHeader, options.trimCells);
        const std::vector<ColumnBinding> bindings = bindColumns(schema, *doc, file, options);

        std::vector<Record> records;
        records.reserve(doc->GetRowCount());

        for (std::size_t row = 0; row < doc->GetRowCount(); ++row) {
            const std::vector<std::string> raw = doc->GetRow<std::string>(row);

            if (options.skipEmptyRows &&
                std::all_of(raw.begin(), raw.end(),
                            [](const std::string& cell) { return trim(cell).empty(); }))
                continue;

            Record record;
            record.className = schema.className;
            record.sourceFile = file.string();
            record.dataRow = row;

            for (const ColumnBinding& binding : bindings) {
                const Field& field = schema.fields[binding.fieldIndex];
                std::string cell;
                if (binding.columnIndex && *binding.columnIndex < raw.size())
                    cell = raw[*binding.columnIndex];

                CellContext ctx{record.sourceFile, field.name, row};
                record.fields.emplace_back(field.name, parseCell(cell, field, ctx));
            }
            records.push_back(std::move(record));
        }
        return records;
    }

    template <typename T>
    std::vector<std::shared_ptr<T>> instantiateFile(const std::filesystem::path& file,
                                                    const std::string& className,
                                                    const LoadOptions& options = {}) const {
        std::vector<Record> records = mapFile(file, className, options);
        std::vector<std::shared_ptr<T>> objects;
        objects.reserve(records.size());
        for (const Record& record : records)
            objects.push_back(factory_.create<T>(record.className, record));
        return objects;
    }

private:
    struct ColumnBinding {
        std::size_t fieldIndex;
        std::string fieldName;
        std::optional<std::size_t> columnIndex;
        std::string columnName;
    };

    std::vector<ColumnBinding> bindColumns(const Schema& schema,
                                           const rapidcsv::Document& doc,
                                           const std::filesystem::path& file,
                                           const LoadOptions& options) const {
        std::vector<std::string> header;
        if (options.hasHeader) header = doc.GetColumnNames();

        std::vector<ColumnBinding> bindings;
        bindings.reserve(schema.fields.size());
        std::vector<std::string> missing;

        if (options.hasHeader) {
            std::unordered_map<std::string, std::size_t> headerIndex;
            for (std::size_t i = 0; i < header.size(); ++i) {
                std::string name = trim(header[i]);
                if (name.empty())
                    throw SchemaMismatchError(file.string() + ": header row contains an empty column name at index " +
                                              std::to_string(i));
                if (!headerIndex.emplace(toLower(name), i).second)
                    throw SchemaMismatchError(file.string() + ": duplicate column name '" + name + "'");
            }

            for (std::size_t f = 0; f < schema.fields.size(); ++f) {
                const Field& field = schema.fields[f];
                const auto it = headerIndex.find(toLower(field.name));
                if (it == headerIndex.end()) {
                    if (field.required()) missing.push_back(field.name);
                    ColumnBinding binding{f, field.name, std::nullopt, field.name};
                    bindings.push_back(std::move(binding));
                    continue;
                }
                ColumnBinding binding{f, field.name, it->second, header[it->second]};
                bindings.push_back(std::move(binding));
            }

            if (!missing.empty()) {
                std::string list;
                for (const std::string& name : missing)
                    list += std::string(list.empty() ? "" : ", ") + "'" + name + "'";
                throw SchemaMismatchError(file.string() + ": CSV is missing required column(s) for class '" +
                                          schema.className + "': " + list);
            }

            std::string extraList;
            for (const std::string& name : header) {
                const std::string trimmed = trim(name);
                if (!schema.findField(trimmed)) {
                    if (!extraList.empty()) extraList += ", ";
                    extraList += "'" + trimmed + "'";
                }
            }
            if (!extraList.empty()) {
                if (!options.ignoreExtraColumns)
                    throw SchemaMismatchError(file.string() + ": CSV has column(s) not defined in class '" +
                                              schema.className + "': " + extraList);
                warn(file.string() + ": ignoring column(s) not defined in class '" +
                     schema.className + "': " + extraList);
            }
            return bindings;
        }

        const std::size_t columnCount = doc.GetColumnCount();
        for (std::size_t f = 0; f < schema.fields.size(); ++f) {
            const Field& field = schema.fields[f];
            if (f < columnCount) {
                ColumnBinding binding{f, field.name, f, "column" + std::to_string(f)};
                bindings.push_back(std::move(binding));
                continue;
            }
            if (field.required()) missing.push_back(field.name);
            ColumnBinding binding{f, field.name, std::nullopt, "column" + std::to_string(f)};
            bindings.push_back(std::move(binding));
        }

        if (!missing.empty()) {
            std::string list;
            for (const std::string& name : missing)
                list += std::string(list.empty() ? "" : ", ") + "'" + name + "'";
            throw SchemaMismatchError(file.string() + ": CSV has too few columns for class '" +
                                      schema.className + "': missing " + list);
        }

        if (schema.fields.size() < columnCount) {
            std::string list;
            for (std::size_t c = schema.fields.size(); c < columnCount; ++c)
                list += std::string(list.empty() ? "" : ", ") + "'column" + std::to_string(c) + "'";
            if (!options.ignoreExtraColumns)
                throw SchemaMismatchError(file.string() + ": CSV has more columns (" +
                                          std::to_string(columnCount) + ") than class '" +
                                          schema.className + "' defines (" +
                                          std::to_string(schema.fields.size()) + ")");
            warn(file.string() + ": ignoring " + std::to_string(columnCount - schema.fields.size()) +
                 " extra column(s) beyond class '" + schema.className + "' fields");
        }
        return bindings;
    }

    void warn(const std::string& message) const {
        if (warn_) warn_(message);
    }

    SchemaBook schemas_;
    ObjectFactory factory_;
    std::function<void(const std::string&)> warn_;
};

}  // namespace csvmap
