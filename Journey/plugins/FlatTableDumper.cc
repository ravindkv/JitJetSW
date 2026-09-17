// FlatTableDumper: prints every NanoAOD FlatTable found in the event (all columns of all
// rows), i.e. exactly the numbers that end up in the NANOAOD tree.
#include <algorithm>

#include "FWCore/Framework/interface/one/EDAnalyzer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/Framework/interface/GetterOfProducts.h"
#include "FWCore/Framework/interface/ProcessMatch.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "DataFormats/NanoAOD/interface/FlatTable.h"

#include "DumpUtil.h"

using namespace chaindump;

class FlatTableDumper : public edm::one::EDAnalyzer<> {
public:
  explicit FlatTableDumper(const edm::ParameterSet& ps)
      : step_(optString(ps, "step", "NANO")), maxElements_(optInt(ps, "maxElements", -1)), getter_(edm::ProcessMatch("*"), this) {
    callWhenNewProductsRegistered(getter_);
  }
  void analyze(const edm::Event& ev, const edm::EventSetup&) override;

private:
  std::string step_;
  int maxElements_;
  edm::GetterOfProducts<nanoaod::FlatTable> getter_;
};

void FlatTableDumper::analyze(const edm::Event& ev, const edm::EventSetup&) {
  std::ostream& os = std::cout;
  eventHeader(os, ev, step_, moduleDescription().moduleLabel());
  std::vector<edm::Handle<nanoaod::FlatTable>> tables;
  getter_.fillHandles(ev, tables);
  std::sort(tables.begin(), tables.end(), [](const edm::Handle<nanoaod::FlatTable>& a, const edm::Handle<nanoaod::FlatTable>& b) {
    if (a->name() != b->name())
      return a->name() < b->name();
    if (a->extension() != b->extension())
      return !a->extension();
    return a.provenance()->moduleLabel() < b.provenance()->moduleLabel();
  });
  os << "\n== FlatTables: n=" << tables.size() << "\n";
  for (auto const& h : tables) {
    const nanoaod::FlatTable& t = *h;
    os << "\n== FlatTable " << t.name() << (t.extension() ? " (extension)" : "") << (t.singleton() ? " (singleton)" : "") << " producer=" << h.provenance()->moduleLabel()
       << " nRows=" << t.nRows() << " nColumns=" << t.nColumns() << " doc=\"" << t.doc() << "\"\n";
    os << "  columns:";
    for (unsigned c = 0; c < t.nColumns(); ++c)
      os << " " << t.columnName(c);
    os << "\n";
    size_t lim = cap(maxElements_, t.nRows());
    for (size_t r = 0; r < lim; ++r) {
      os << "  " << t.name() << "[" << r << "]";
      for (unsigned c = 0; c < t.nColumns(); ++c) {
        os << " " << t.columnName(c) << "=";
        switch (t.columnType(c)) {
          case nanoaod::FlatTable::ColumnType::Float:
            os << t.columnData<float>(c)[r];
            break;
          case nanoaod::FlatTable::ColumnType::Double:
            os << t.columnData<double>(c)[r];
            break;
          case nanoaod::FlatTable::ColumnType::Int32:
            os << t.columnData<int32_t>(c)[r];
            break;
          case nanoaod::FlatTable::ColumnType::UInt32:
            os << t.columnData<uint32_t>(c)[r];
            break;
          case nanoaod::FlatTable::ColumnType::Int16:
            os << t.columnData<int16_t>(c)[r];
            break;
          case nanoaod::FlatTable::ColumnType::UInt16:
            os << t.columnData<uint16_t>(c)[r];
            break;
          case nanoaod::FlatTable::ColumnType::UInt8:
            os << static_cast<int>(t.columnData<uint8_t>(c)[r]);
            break;
          case nanoaod::FlatTable::ColumnType::Bool:
            os << static_cast<int>(t.columnData<bool>(c)[r]);
            break;
          case nanoaod::FlatTable::ColumnType::Int64:
            os << t.columnData<int64_t>(c)[r];
            break;
          case nanoaod::FlatTable::ColumnType::UInt64:
            os << t.columnData<uint64_t>(c)[r];
            break;
          default:
            os << t.getAnyValue(r, c);
        }
      }
      os << "\n";
    }
    truncated(os, lim, t.nRows());
  }
  os.flush();
}

DEFINE_FWK_MODULE(FlatTableDumper);
