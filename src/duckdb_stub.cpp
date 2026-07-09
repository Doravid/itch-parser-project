namespace duckdb
{
    class DuckDB;
    class ExtensionHelper
    {
    public:
        static void LoadAllExtensions(DuckDB &db);
    };

    void ExtensionHelper::LoadAllExtensions(DuckDB &db) {}
}