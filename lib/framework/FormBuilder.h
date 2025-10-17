#ifndef FORMBUILDER_H
#define FORMBUILDER_H

#include <ArduinoJson.h>
#include <vector>
#include <math.h>  // для sin, cos

// ------------------- Глобальні константи для зручності -------------------
static constexpr bool hidden = true;
static constexpr bool visible = false;

// ====================== Теги для NUMBER ======================
struct MinVal { double v; };
inline MinVal minVal(double v) { return MinVal{v}; }

struct MaxVal { double v; };
inline MaxVal maxVal(double v) { return MaxVal{v}; }

struct Format { const char* f; };
inline Format format(const char* f) { return Format{f}; }

// ====================== Теги для SLIDER / COMMON PLACEHOLDER ======================
struct Step { double v; };  // st
inline Step step(double v) { return Step{v}; }

struct Placeholder { const char* t; };  // pl
inline Placeholder placeholder(const char* t) { return Placeholder{t}; }

// ====================== Теги для OPTIONS (dropdown/radio) ======================
struct Opt {
  const char* label;
  int value;
};
inline Opt opt(const char* lbl, int val) { return Opt{lbl, val}; }

// ====================== TREND: line(), xAxis(), lines(), legend(), tooltip(), trendMaxPoints(), mode()  =====
struct TrendLine {
  const char* keyName;
  bool hidden;
  const char* color;
  const char* type;
};

inline TrendLine line(const char* keyName, bool h, const char* color, const char* type) {
  return {keyName, h, color, type};
}
inline TrendLine line(const char* keyName, const char* color, const char* type) {
  return {keyName, false, color, type};
}

struct TrendXAxis { const char* xAxisKey; };
inline TrendXAxis xAxis(const char* v) { return TrendXAxis{v}; }

struct TrendLines { const char* config; };
inline TrendLines lines(const char* c) { return TrendLines{c}; }

struct TrendLegend { bool show; };
inline TrendLegend legend(bool on) { return TrendLegend{on}; }

struct TrendTooltip { bool show; };
inline TrendTooltip tooltip(bool on) { return TrendTooltip{on}; }

struct TrendMaxPoints { int value; };
inline TrendMaxPoints trendMaxPoints(int v) { return TrendMaxPoints{v}; }

struct TrendMode { const char* mode; };  // "lineChart" або "barChart"
inline TrendMode mode(const char* m) { return TrendMode{m}; }

// ====================== Парсер варіативних TREND-аргументів  ======================
static void parseLineArg(String& out, const TrendLine& ln) {
  if (out.length() > 0) out += ";";
  out += ln.keyName;
  out += ":";
  if (ln.hidden) out += "hidden=true,";
  out += "color="; out += ln.color;
  out += ",type="; out += ln.type;
}
static void parseLineArgs(String&) {}
template <typename First, typename... Rest>
static void parseLineArgs(String& out, First first, Rest... rest) {
  parseLineArg(out, first);
  parseLineArgs(out, rest...);
}
template <typename... Args>
static TrendLines lines(Args... lineObjs) {
  static String buffer;
  buffer = "";
  parseLineArgs(buffer, lineObjs...);
  return TrendLines{buffer.c_str()};
}

// ====================== Перелік типів поля ======================
enum class AF { R, RW };
inline const char* toString(AF af) {
  switch (af) { case AF::R: return "r"; case AF::RW: return "rw"; }
  return "r";
}

enum class FieldType {
  TEXT, NUMBER, SLIDER, CHECKBOX, SWITCH, DROPDOWN, TEXTAREA, RADIO, TREND, BUTTON, UNKNOWN
};
inline const char* toString(FieldType ft) {
  switch (ft) {
    case FieldType::TEXT:     return "text";
    case FieldType::NUMBER:   return "number";
    case FieldType::SLIDER:   return "slider";
    case FieldType::CHECKBOX: return "checkbox";
    case FieldType::SWITCH:   return "switch";
    case FieldType::DROPDOWN: return "dropdown";
    case FieldType::TEXTAREA: return "textarea";
    case FieldType::RADIO:    return "radio";
    case FieldType::TREND:    return "trend";
    case FieldType::BUTTON:   return "button";
    case FieldType::UNKNOWN:  return "unknown";
  }
  return "unknown";
}

// ====================== Парсер для number field ======================
struct MinVal;  // forward
static void parseNumberArg(MinVal m, double& mn, double&, const char*&) { mn = m.v; }
struct MaxVal;  // forward
static void parseNumberArg(MaxVal m, double&, double& mx, const char*&) { mx = m.v; }
struct Format;  // forward
static void parseNumberArg(Format f, double&, double&, const char*& fmt) { fmt = f.f; }

static void parseNumberArgs(double&, double&, const char*&) {}
template <typename Arg>
static void parseNumberArgs(double& mn, double& mx, const char*& fmt, Arg arg) {
  parseNumberArg(arg, mn, mx, fmt);
  parseNumberArgs(mn, mx, fmt);
}
template <typename Arg, typename... Args>
static void parseNumberArgs(double& mn, double& mx, const char*& fmt, Arg arg, Args... rest) {
  parseNumberArg(arg, mn, mx, fmt);
  parseNumberArgs(mn, mx, fmt, rest...);
}

// ====================== Парсер для SLIDER (варіативні теги) ======================
static void parseSliderArg(MinVal m, double& mn, double&, double&, const char*&) { mn = m.v; }
static void parseSliderArg(MaxVal m, double&, double& mx, double&, const char*&) { mx = m.v; }
static void parseSliderArg(Step s, double&, double&, double& st, const char*&) { st = s.v; }
static void parseSliderArg(Placeholder p, double&, double&, double&, const char*& pl) { pl = p.t; }
static void parseSliderArgs(double&, double&, double&, const char*&) {}
template <typename Arg>
static void parseSliderArgs(double& mn, double& mx, double& st, const char*& pl, Arg arg) {
  parseSliderArg(arg, mn, mx, st, pl);
  parseSliderArgs(mn, mx, st, pl);
}
template <typename Arg, typename... Args>
static void parseSliderArgs(double& mn, double& mx, double& st, const char*& pl, Arg arg, Args... rest) {
  parseSliderArg(arg, mn, mx, st, pl);
  parseSliderArgs(mn, mx, st, pl, rest...);
}

// ====================== Парсер для BUTTON (опційно тільки placeholder) ======================
static void parseButtonArg(Placeholder p, const char*& pl) { pl = p.t; }
static void parseButtonArgs(const char*&) {}
template <typename Arg>
static void parseButtonArgs(const char*& pl, Arg arg) {
  parseButtonArg(arg, pl);
  parseButtonArgs(pl);
}
template <typename Arg, typename... Args>
static void parseButtonArgs(const char*& pl, Arg arg, Args... rest) {
  parseButtonArg(arg, pl);
  parseButtonArgs(pl, rest...);
}

// ====================== Парсер для dropdown/radio options ======================
static void parseOptionsArg(Opt o, std::vector<Opt>& opts) { opts.push_back(o); }
static void parseOptionsArgs(std::vector<Opt>&) {}
template <typename Arg, typename... Args>
static void parseOptionsArgs(std::vector<Opt>& opts, Arg arg, Args... rest) {
  parseOptionsArg(arg, opts);
  parseOptionsArgs(opts, rest...);
}

// ====================== Парсер для TREND (варіативні теги) ======================
static void parseTrendArg(TrendXAxis t, String& xAxisKey, String&, bool&, bool&, int&, String&) {
  xAxisKey = t.xAxisKey;
}
static void parseTrendArg(TrendLines l, String&, String& linesStr, bool&, bool&, int&, String&) {
  linesStr = l.config;
}
static void parseTrendArg(TrendLegend lg, String&, String&, bool& showLegend, bool&, int&, String&) {
  showLegend = lg.show;
}
static void parseTrendArg(TrendTooltip tt, String&, String&, bool&, bool& showTooltip, int&, String&) {
  showTooltip = tt.show;
}
static void parseTrendArg(TrendMaxPoints p, String&, String&, bool&, bool&, int& maxPoints, String&) {
  maxPoints = p.value;
}
static void parseTrendArg(TrendMode m, String&, String&, bool&, bool&, int&, String& modeStr) {
  modeStr = m.mode;
}

static void parseTrendArgs(String&, String&, bool&, bool&, int&, String&) {}
template <typename Arg, typename... Args>
static void parseTrendArgs(String& xAxisKey, String& linesStr, bool& showLegend, bool& showTooltip, int& maxPoints, String& modeStr, Arg arg, Args... rest) {
  parseTrendArg(arg, xAxisKey, linesStr, showLegend, showTooltip, maxPoints, modeStr);
  parseTrendArgs(xAxisKey, linesStr, showLegend, showTooltip, maxPoints, modeStr, rest...);
}

// ====================== FormBuilder ======================
class FormBuilder {
 public:
  static JsonArray createForm(JsonObject& root, const char* name, const char* description) {
    JsonObject form = root.createNestedObject(name);
    form["description"] = description;
    return form.createNestedArray("fields");
  }

  // Загальний оновлювач (для небулевих типів)
  template <typename T>
  static bool updateValue(JsonObject& root, const char* key, T& value) {
    if (root.containsKey(key) && root[key].is<T>()) {
      T newValue = root[key].as<T>();
      if (newValue != value) { value = newValue; return true; }
    }
    return false;
  }

  // Спеціальний оновлювач для bool — приймаємо ТІЛЬКИ справжній JSON bool
  static bool updateValue(JsonObject& root, const char* key, bool& value) {
    if (root.containsKey(key) && root[key].is<bool>()) {
      bool nv = root[key].as<bool>();
      if (nv != value) { value = nv; return true; }
    }
    return false;
  }

  // ---------- TEXT ----------
  static JsonObject addTextField(JsonArray& fields, const char* key, AF accessFlag, const char* val) {
    JsonObject field = fields.createNestedObject();
    field[key] = val;
    setBasicOptions(field, FieldType::TEXT, accessFlag);
    return field;
  }

  // ---------- NUMBER ----------
  template <typename... Args>
  static JsonObject addNumberField(JsonArray& fields, const char* key, AF accessFlag, double value, Args... numberArgs) {
    double mn = 0, mx = 100; const char* fmt = "";
    parseNumberArgs(mn, mx, fmt, numberArgs...);

    JsonObject field = fields.createNestedObject();
    field[key] = String(value);  // сумісність із поточним фронтом (value як рядок)

    String oValue = String(toString(FieldType::NUMBER)) + ";" + toString(accessFlag);
    oValue += ";mn="; oValue += mn;
    oValue += ";mx="; oValue += mx;
    if (fmt && fmt[0] != '\0') { oValue += ";f="; oValue += fmt; }

    field["o"] = oValue;
    return field;
  }

  // ---------- SLIDER ----------
  template <typename... Args>
  static JsonObject addSliderField(JsonArray& fields, const char* key, AF accessFlag, double value, Args... sliderArgs) {
    double mn = 0, mx = 100, st = 1; const char* pl = nullptr;
    parseSliderArgs(mn, mx, st, pl, sliderArgs...);

    JsonObject field = fields.createNestedObject();
    field[key] = value; // числом

    String oValue = String(toString(FieldType::SLIDER)) + ";" + toString(accessFlag);
    oValue += ";mn="; oValue += mn;
    oValue += ";mx="; oValue += mx;
    if (!std::isnan(st)) { oValue += ";st="; oValue += st; }
    if (pl && pl[0] != '\0') { oValue += ";pl="; oValue += pl; }

    field["o"] = oValue;
    return field;
  }

  // ---------- CHECKBOX (boolean) ----------
  static JsonObject addCheckboxField(JsonArray& fields, const char* key, AF accessFlag, bool value) {
    JsonObject field = fields.createNestedObject();
    field[key] = value;  // ЛИШЕ true/false
    setBasicOptions(field, FieldType::CHECKBOX, accessFlag);
    return field;
  }

  // ---------- SWITCH (boolean) ----------
  static JsonObject addSwitchField(JsonArray& fields, const char* key, AF accessFlag, bool value) {
    JsonObject field = fields.createNestedObject();
    field[key] = value;  // ЛИШЕ true/false
    setBasicOptions(field, FieldType::SWITCH, accessFlag);
    return field;
  }

  // ---------- BUTTON (boolean) ----------
  template <typename... Args>
  static JsonObject addButtonField(JsonArray& fields, const char* key, AF accessFlag, bool value, Args... btnArgs) {
    const char* pl = nullptr;
    parseButtonArgs(pl, btnArgs...);

    JsonObject field = fields.createNestedObject();
    field[key] = value;  // ЛИШЕ true/false

    String oValue = String(toString(FieldType::BUTTON)) + ";" + toString(accessFlag);
    if (pl && pl[0] != '\0') { oValue += ";pl="; oValue += pl; }

    field["o"] = oValue;
    return field;
  }

  // ---------- DROPDOWN ----------
  template <typename... Args>
  static JsonObject addDropdownField(JsonArray& fields, const char* key, AF accessFlag, int selectedValue, Args... optArgs) {
    JsonObject field = fields.createNestedObject();
    field[key] = selectedValue;

    std::vector<Opt> options;
    parseOptionsArgs(options, optArgs...);

    String oValue = String(toString(FieldType::DROPDOWN)) + ";" + toString(accessFlag);
    if (!options.empty()) {
      oValue += ";options=";
      bool first = true;
      for (auto& op : options) {
        if (!first) oValue += ",";
        oValue += op.label;
        first = false;
      }
    }

    field["o"] = oValue;
    return field;
  }

  // ---------- TEXTAREA ----------
  static JsonObject addTextareaField(JsonArray& fields, const char* key, AF accessFlag, const String& value) {
    JsonObject field = fields.createNestedObject();
    field[key] = value;
    setBasicOptions(field, FieldType::TEXTAREA, accessFlag);
    return field;
  }

  // ---------- RADIO ----------
  template <typename... Args>
  static JsonObject addRadioField(JsonArray& fields, const char* key, AF accessFlag, int selectedValue, Args... optArgs) {
    JsonObject field = fields.createNestedObject();
    field[key] = selectedValue;

    std::vector<Opt> options;
    parseOptionsArgs(options, optArgs...);

    String oValue = String(toString(FieldType::RADIO)) + ";" + toString(accessFlag);
    if (!options.empty()) {
      oValue += ";options=";
      bool first = true;
      for (auto& op : options) {
        if (!first) oValue += ",";
        oValue += op.label;
        first = false;
      }
    }

    field["o"] = oValue;
    return field;
  }

  // ---------- TREND ----------
  template <typename... Args>
  static JsonObject addTrendField(JsonArray& fields, const char* key, AF accessFlag, Args... trendArgs) {
    JsonObject field = fields.createNestedObject();
    JsonObject trendObj = field.createNestedObject(key); (void)trendObj; // зарезервовано для сумісності

    String xAxisKeyStr = "timestamp";
    String linesStr;
    bool showLegend = false, showTooltip = false;
    int maxPts = 100;
    String modeStr = "lineChart";

    parseTrendArgs(xAxisKeyStr, linesStr, showLegend, showTooltip, maxPts, modeStr, trendArgs...);

    String oValue = String(toString(FieldType::TREND)) + ";" + toString(accessFlag);
    oValue += ";mode=";  oValue += modeStr;
    if (modeStr == "barChart") { oValue += ";xAxis=keys"; }
    else { oValue += ";xAxis="; oValue += xAxisKeyStr; }
    if (linesStr.length() > 0) { oValue += ";lines="; oValue += linesStr; }
    if (showLegend)  oValue += ";legend=true";
    if (showTooltip) oValue += ";tooltip=true";
    if (maxPts > 0)  { oValue += ";maxPoints="; oValue += maxPts; }

    field["o"] = oValue;
    return field;
  }

  // ---------- DEFAULT ----------
  static JsonObject addDefaultField(JsonArray& fields, const char* key, AF accessFlag, const char* value) {
    JsonObject field = fields.createNestedObject();
    field[key] = value;
    setBasicOptions(field, FieldType::UNKNOWN, accessFlag);
    return field;
  }

 private:
  static void setBasicOptions(JsonObject& field, FieldType ft, AF af) {
    String oValue = String(toString(ft)) + ";" + toString(af);
    field["o"] = oValue;
  }
};

#endif  // FORMBUILDER_H
