#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "csv_adapter.hpp"
#include "person.hpp"

namespace fs = std::filesystem;

namespace {

void banner(const std::string& title) {
    std::cout << "\n=== " << title << " ===\n";
}

void printRecords(const std::vector<csvmap::Record>& records) {
    for (const csvmap::Record& record : records) {
        std::cout << "  [" << record.className << " | row " << (record.dataRow + 1) << "] ";
        for (std::size_t i = 0; i < record.fields.size(); ++i) {
            if (i > 0) std::cout << " | ";
            std::cout << record.fields[i].first << "=" << record.fields[i].second;
        }
        std::cout << "\n";
    }
}

template <typename T>
void printInstances(const std::string& className, const std::vector<std::shared_ptr<T>>& objects) {
    for (const std::shared_ptr<T>& object : objects)
        std::cout << "  " << className << ": " << *object << "\n";
}

void expectFailure(const std::string& what, const std::function<void()>& action) {
    try {
        action();
    } catch (const csvmap::CsvAdapterError& e) {
        std::cout << "  [" << what << "]\n  " << e.what() << "\n";
        return;
    }
    std::cout << "  [" << what << "] expected a failure but the call succeeded\n";
}

}  // namespace

int main(int argc, char** argv) {
    const fs::path base = (argc > 1) ? fs::path(argv[1]) : fs::path(".");
    std::cout << "CSV adapter demo (base dir: " << fs::absolute(base).string() << ")\n";

    try {
        banner("1. Scan the schema folder for class definitions");
        csvmap::SchemaBook book;
        const std::size_t found = book.scanFolder(base / "schemas");
        std::cout << "  discovered " << found << " class definition(s):\n";
        for (const std::string& name : book.classNames()) {
            const csvmap::Schema& schema = book.get(name);
            std::cout << "    class " << schema.className << " (from " << schema.source << ")\n";
            for (const csvmap::Field& field : schema.fields) {
                std::cout << "      " << field.name << " : " << csvmap::fieldTypeName(field.type);
                if (field.optional) std::cout << " (optional)";
                if (field.hasDefault) std::cout << " (default " << field.defaultValue << ")";
                std::cout << "\n";
            }
        }

        csvmap::ObjectFactory factory;
        factory.registerClass<Person>("person", &Person::fromRecord);
        csvmap::CsvAdapter adapter(std::move(book), std::move(factory));
        adapter.setWarningHandler([](const std::string& message) {
            std::cout << "  [warning] " << message << "\n";
        });

        banner("2. Map data/people.csv (comma) into generic Records");
        printRecords(adapter.mapFile(base / "data/people.csv", "person"));

        banner("3. Instantiate typed Person objects from data/people_semicolon.csv");
        csvmap::LoadOptions semicolonOptions;
        semicolonOptions.delimiter = csvmap::Delimiter::Semicolon;
        printRecords(adapter.mapFile(base / "data/people_semicolon.csv", "person", semicolonOptions));
        printInstances("Person", adapter.instantiateFile<Person>(
                                      base / "data/people_semicolon.csv", "person", semicolonOptions));

        banner("4. Delimiter auto-detection: data/people_tabs.tsv (tab)");
        printRecords(adapter.mapFile(base / "data/people_tabs.tsv", "person"));

        banner("5. Schema-declared delimiter: data/lab.csv uses ';' via measurement.schema");
        printRecords(adapter.mapFile(base / "data/lab.csv", "measurement"));

        banner("6. Extra CSV column is ignored with a warning by default");
        printRecords(adapter.mapFile(base / "data/people_extra_column.csv", "person"));

        banner("7. Strict mode rejects extra columns");
        csvmap::LoadOptions strict;
        strict.ignoreExtraColumns = false;
        expectFailure("schema mismatch (extra column)", [&] {
            adapter.mapFile(base / "data/people_extra_column.csv", "person", strict);
        });

        banner("8. Missing required columns are a schema mismatch");
        expectFailure("schema mismatch (missing columns)", [&] {
            adapter.mapFile(base / "data/people_missing_columns.csv", "person");
        });

        banner("9. Bad cell values and short rows are reported per row and column");
        expectFailure("cell parse error", [&] {
            adapter.mapFile(base / "data/people_bad_values.csv", "person");
        });
        expectFailure("ragged row", [&] {
            adapter.mapFile(base / "data/people_short_row.csv", "person");
        });

        banner("10. Factory errors surface as typed errors");
        expectFailure("no factory registered", [&] {
            adapter.instantiateFile<Person>(base / "data/lab.csv", "measurement");
        });
        expectFailure("incompatible target type", [&] {
            adapter.instantiateFile<int>(base / "data/people.csv", "person");
        });

        banner("11. Unknown class names are rejected against the scanned schemas");
        expectFailure("unknown class", [&] {
            adapter.mapFile(base / "data/people.csv", "contractor");
        });
    } catch (const std::exception& e) {
        std::cerr << "unhandled error: " << e.what() << "\n";
        return 1;
    }

    std::cout << "\nAll demo scenarios completed.\n";
    return 0;
}
