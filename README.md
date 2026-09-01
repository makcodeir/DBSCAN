# DBSCAN

An open-source implementation of DBSCAN in C++, with a schema-driven CSV adapter that maps CSV rows into value classes.

## CSV adapter (`csv_adapter.hpp`)

The adapter automates parsing CSV files (via the bundled [rapidcsv](https://github.com/d99kris/rapidcsv) header `rapidcsv.h`) and mapping their contents into value classes.

Core pieces:

| Component | Purpose |
|---|---|
| `value` (`value.hpp`) | Variant-based value type: null, bool, int, double, char, string |
| `SchemaBook` | Scans a folder for `.schema` class-definition files and loads them |
| `Schema` / `Field` | A discovered class definition: field names, types, optional/default flags |
| `CsvAdapter` | Loads a CSV, binds its columns to a schema, and produces one `Record` per row |
| `Record` | A mapped row: named `value` fields in schema order |
| `ObjectFactory` | Registers builder functions that turn `Record`s into your C++ classes |
| `LoadOptions` | Delimiter, header mode, cell trimming, extra-column policy, empty-row skipping |

### Schema files

The adapter scans a folder for `*.schema` files (filename becomes the default class name):

```
# schemas/person.schema
class Person
---
name   : string
age    : int
height : double
active : bool
email  : string?
```

- Types: `bool`, `int`, `double`, `char`, `string`.
- `?` marks a column as optional (missing column or empty cell maps to a null `value`).
- `= <literal>` supplies a default for empty cells (e.g. `age : int = 0`).
- `class <Name>` overrides the class name; `delimiter <char|comma|semicolon|tab|pipe>` binds a default delimiter to the class.
- `#` or `//` lines are comments; `---` lines are ignored.

### Delimiters

`Delimiter::Comma`, `Semicolon`, `Tab`, `Pipe`, or `Auto` (sniffs the header line, quote-aware). Precedence: explicit `LoadOptions::delimiter` > schema `delimiter` directive > auto-detection. Quoted fields (RFC 4180 style) are handled by rapidcsv, including separators and escaped quotes inside quotes.

### Error handling

All errors derive from `csvmap::CsvAdapterError` (derived from `std::runtime_error`):

- `SchemaError` - malformed schema file, duplicate class definitions, unknown class name.
- `SchemaMismatchError` - CSV vs. schema mismatch: missing required columns, extra columns in strict mode, too few columns.
- `CellParseError` - a cell cannot be converted (message names file, data row, column, raw text) or a required value is missing (e.g. ragged/short rows).
- `FactoryError` - no factory registered for a class, or the registered factory returns an incompatible type.

By default extra columns are ignored with a warning (route via `setWarningHandler`); set `LoadOptions::ignoreExtraColumns = false` to make them errors.

### Example

Schema (`schemas/person.schema`) as above, CSV `data/people.csv`:

```
name,age,height,active,email
Alice,30,1.70,true,alice@example.com
Bob,25,1.83,false,
```

Custom class and mapping (`person.hpp`, `main.cpp`):

```cpp
class Person {
public:
    std::string name;
    int age = 0;
    double height = 0.0;
    bool active = false;
    std::string email;

    static std::shared_ptr<Person> fromRecord(const csvmap::Record& record) {
        auto person = std::make_shared<Person>();
        person->name   = record.at("name").get<std::string>();
        person->age    = record.at("age").get<int>();
        person->height = record.at("height").asDouble();
        person->active = record.at("active").get<bool>();
        const value& email = record.at("email");
        if (!email.isNull()) person->email = email.get<std::string>();
        return person;
    }
};

int main() {
    csvmap::SchemaBook book;
    book.scanFolder("schemas");                    // discovers person.schema, measurement.schema

    csvmap::ObjectFactory factory;
    factory.registerClass<Person>("person", &Person::fromRecord);
    csvmap::CsvAdapter adapter(std::move(book), std::move(factory));

    for (const auto& person : adapter.instantiateFile<Person>("data/people.csv", "person"))
        std::cout << *person << "\n";
}
```

Rows may also be consumed generically via `adapter.mapFile(...)` which returns `std::vector<Record>`.

### Build and run the demo

```
cmake -S . -B build
cmake --build build
./build/csv_adapter_demo .
```

The demo (`main.cpp`) exercises generic row mapping, typed instantiation, all delimiter modes, and every error class.

### Notes and limitations

- Requires C++17 (`std::variant`, `std::optional`, `std::any`, `<filesystem>`).
- Files with a UTF-8 BOM: the BOM becomes part of the first header name and will be reported as a missing column.
- Cell values are parsed from trimmed text; `bool` accepts `true/false, yes/no, y/n, t/f, 1/0` (case-insensitive).
