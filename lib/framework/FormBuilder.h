#ifndef FORMBUILDER_H
#define FORMBUILDER_H

#include <ArduinoJson.h>
#include <vector>
#include <math.h>  // для sin, cos

// ------------------- Глобальні константи для зручності -------------------
static constexpr bool hidden = true;
static constexpr bool visible = false;

// ====================== Теги для NUMBER ======================
struct MinVal {
  double v;
};
inline MinVal minVal(double v) {
  return MinVal{v};
}

struct MaxVal {
  double v;
};
inline MaxVal maxVal(double v) {
  return MaxVal{v};
}

struct Format {
  const char* f;
};
inline Format format(const char* f) {
  return Format{f};
}

// ====================== Теги для SLIDER ======================
struct Step {
  double v;
};  // st
inline Step step(double v) {
  return Step{v};
}

struct Placeholder {
  const char* t;
};  // pl
inline Placeholder placeholder(const char* t) {
  return Placeholder{t};
}

// ====================== Теги для OPTIONS (dropdown/radio) ======================
struct Opt {
  const char* label;
  int value;
};
inline Opt opt(const char* lbl, int val) {
  return Opt{lbl, val};
}

// ====================== TREND: line(), xAxis(), lines(), legend(), tooltip(), trendMaxPoints(), mode()  =====

/**
 * Опис однієї лінії, де:
 *  - keyName: "key1", "key2"...
 *  - hidden:  true/false (чи приховувати цей ключ)
 *  - color:   "#8884d8"
 *  - type:    "monotone", "step", ...
 */
struct TrendLine {
  const char* keyName;
  bool hidden;
  const char* color;
  const char* type;
};

/**
 * line(...):
 * 1) Якщо хочемо явно вказати hidden=true/false:
 *    line("key1", hidden, "#8884d8", "monotone")
 *    line("key2", visible, "#FF0000", "monotone")
 *
 * 2) Або базова версія (без булевого аргументу):
 *    line("key1", "#8884d8", "monotone") => hidden = false за замовчуванням
 */
inline TrendLine line(const char* keyName, bool h, const char* color, const char* type) {
  return {keyName, h, color, type};
}

inline TrendLine line(const char* keyName, const char* color, const char* type) {
  return {keyName, false, color, type};
}

/** TrendXAxis */
struct TrendXAxis {
  const char* xAxisKey;
};
inline TrendXAxis xAxis(const char* v) {
  return TrendXAxis{v};
}

/** TrendLines – рядок, що зберігає усі лінії */
struct TrendLines {
  const char* config;
};
inline TrendLines lines(const char* c) {
  return TrendLines{c};
}

/** TrendLegend */
struct TrendLegend {
  bool show;
};
inline TrendLegend legend(bool on) {
  return TrendLegend{on};
}

/** TrendTooltip */
struct TrendTooltip {
  bool show;
};
inline TrendTooltip tooltip(bool on) {
  return TrendTooltip{on};
}

/** TrendMaxPoints */
struct TrendMaxPoints {
  int value;
};
inline TrendMaxPoints trendMaxPoints(int v) {
  return TrendMaxPoints{v};
}

/** TrendMode */
struct TrendMode {
  const char* mode;  // "lineChart" або "barChart"
};
inline TrendMode mode(const char* m) {
  return TrendMode{m};
}

// ====================== Парсер варіативних TREND-аргументів  ======================

/**
 * parseLineArg(...) – формує шматок для lines=...
 * Формат:   keyName:hidden=true,color=#8884d8,type=monotone
 */
static void parseLineArg(String& out, const TrendLine& ln) {
  if (out.length() > 0) {
    out += ";";
  }
  out += ln.keyName;
  out += ":";

  if (ln.hidden) {
    out += "hidden=true,";
  }
  // Якщо hidden=false, можемо не записувати "hidden=false" узагалі.

  out += "color=";
  out += ln.color;
  out += ",type=";
  out += ln.type;
}

// Рекурсія для набору line(...)
static void parseLineArgs(String& out) {
  // завершення
}
template <typename First, typename... Rest>
static void parseLineArgs(String& out, First first, Rest... rest) {
  parseLineArg(out, first);
  parseLineArgs(out, rest...);
}

/**
 * lines(...) приймає довільну кількість TrendLine і склеює їх у єдиний рядок.
 * Приклад: "key1:hidden=true,color=#8884d8,type=monotone;key2:..."
 *
 * УВАГА: Використовує статичний буфер => не можна викликати 2x lines(...) одночасно
 */
template <typename... Args>
static TrendLines lines(Args... lineObjs) {
  static String buffer;
  buffer = "";  // очистити
  parseLineArgs(buffer, lineObjs...);
  return TrendLines{buffer.c_str()};
}

// ====================== Перелік типів поля ======================
enum class AF { R, RW };
inline const char* toString(AF af) {
  switch (af) {
    case AF::R:
      return "r";
    case AF::RW:
      return "rw";
  }
  return "r";
}

enum class FieldType { TEXT, NUMBER, SLIDER, CHECKBOX, SWITCH, DROPDOWN, TEXTAREA, RADIO, TREND, UNKNOWN };
inline const char* toString(FieldType ft) {
  switch (ft) {
    case FieldType::TEXT:
      return "text";
    case FieldType::NUMBER:
      return "number";
    case FieldType::SLIDER:
      return "slider";
    case FieldType::CHECKBOX:
      return "checkbox";
    case FieldType::SWITCH:
      return "switch";
    case FieldType::DROPDOWN:
      return "dropdown";
    case FieldType::TEXTAREA:
      return "textarea";
    case FieldType::RADIO:
      return "radio";
    case FieldType::TREND:
      return "trend";
    case FieldType::UNKNOWN:
      return "unknown";
  }
  return "unknown";
}

// ====================== Парсер для number field ======================
struct MinVal;  // оголошено вище
static void parseNumberArg(MinVal m, double& mn, double&, const char*&) {
  mn = m.v;
}
struct MaxVal;  // оголошено вище
static void parseNumberArg(MaxVal m, double&, double& mx, const char*&) {
  mx = m.v;
}
struct Format;  // оголошено вище
static void parseNumberArg(Format f, double&, double&, const char*& fmt) {
  fmt = f.f;
}
static void parseNumberArgs(double&, double&, const char*&) {
  // завершення
}
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
static void parseSliderArg(MinVal m, double& mn, double&, double&, const char*&) {
  mn = m.v;
}
static void parseSliderArg(MaxVal m, double&, double& mx, double&, const char*&) {
  mx = m.v;
}
static void parseSliderArg(Step s, double&, double&, double& st, const char*&) {
  st = s.v;
}
static void parseSliderArg(Placeholder p, double&, double&, double&, const char*& pl) {
  pl = p.t;
}
static void parseSliderArgs(double&, double&, double&, const char*&) {
  // завершення
}
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

// ====================== Парсер для dropdown/radio options ======================
static void parseOptionsArg(Opt o, std::vector<Opt>& opts) {
  opts.push_back(o);
}
static void parseOptionsArgs(std::vector<Opt>&) {
  // завершення
}
template <typename Arg, typename... Args>
static void parseOptionsArgs(std::vector<Opt>& opts, Arg arg, Args... rest) {
  parseOptionsArg(arg, opts);
  parseOptionsArgs(opts, rest...);
}

// ====================== Парсер для TREND (варіативні теги) ======================
static void parseTrendArg(TrendXAxis t,
                          String& xAxisKey,
                          String& linesStr,
                          bool& showLegend,
                          bool& showTooltip,
                          int& maxPoints,
                          String& modeStr) {
  xAxisKey = t.xAxisKey;
}
static void parseTrendArg(TrendLines l,
                          String& xAxisKey,
                          String& linesStr,
                          bool& showLegend,
                          bool& showTooltip,
                          int& maxPoints,
                          String& modeStr) {
  linesStr = l.config;
}
static void parseTrendArg(TrendLegend lg,
                          String& xAxisKey,
                          String& linesStr,
                          bool& showLegend,
                          bool& showTooltip,
                          int& maxPoints,
                          String& modeStr) {
  showLegend = lg.show;
}
static void parseTrendArg(TrendTooltip tt,
                          String& xAxisKey,
                          String& linesStr,
                          bool& showLegend,
                          bool& showTooltip,
                          int& maxPoints,
                          String& modeStr) {
  showTooltip = tt.show;
}
static void parseTrendArg(TrendMaxPoints p,
                          String& xAxisKey,
                          String& linesStr,
                          bool& showLegend,
                          bool& showTooltip,
                          int& maxPoints,
                          String& modeStr) {
  maxPoints = p.value;
}
static void parseTrendArg(TrendMode m,
                          String& xAxisKey,
                          String& linesStr,
                          bool& showLegend,
                          bool& showTooltip,
                          int& maxPoints,
                          String& modeStr) {
  modeStr = m.mode;
}

// Рекурсія обробки варіативних аргументів
static void parseTrendArgs(String&, String&, bool&, bool&, int&, String&) {
  // завершення
}
template <typename Arg, typename... Args>
static void parseTrendArgs(String& xAxisKey,
                           String& linesStr,
                           bool& showLegend,
                           bool& showTooltip,
                           int& maxPoints,
                           String& modeStr,
                           Arg arg,
                           Args... rest) {
  parseTrendArg(arg, xAxisKey, linesStr, showLegend, showTooltip, maxPoints, modeStr);
  parseTrendArgs(xAxisKey, linesStr, showLegend, showTooltip, maxPoints, modeStr, rest...);
}

// ====================== FormBuilder ======================
class FormBuilder {
 public:
  // Створює JSON-форму
  static JsonArray createForm(JsonObject& root, const char* name, const char* description) {
    JsonObject form = root.createNestedObject(name);
    form["description"] = description;
    return form.createNestedArray("fields");
  }

  template <typename T>
  static bool updateValue(JsonObject& root, const char* key, T& value) {
    if (root.containsKey(key)) {
      T newValue = root[key].as<T>();
      if (newValue != value) {
        value = newValue;
        return true;
      }
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
  static JsonObject addNumberField(JsonArray& fields,
                                   const char* key,
                                   AF accessFlag,
                                   double value,
                                   Args... numberArgs) {
    double mn = 0;
    double mx = 100;
    const char* fmt = "";

    parseNumberArgs(mn, mx, fmt, numberArgs...);

    JsonObject field = fields.createNestedObject();
    field[key] = String(value);

    String oValue = String(toString(FieldType::NUMBER)) + ";" + toString(accessFlag);
    oValue += ";mn=";
    oValue += mn;
    oValue += ";mx=";
    oValue += mx;
    if (fmt && fmt[0] != '\0') {
      oValue += ";f=";
      oValue += fmt;
    }

    field["o"] = oValue;
    return field;
  }

  // ====================== SLIDER ======================
  template <typename... Args>
  static JsonObject addSliderField(JsonArray& fields,
                                   const char* key,
                                   AF accessFlag,
                                   double value,
                                   Args... sliderArgs) {
    // Значення за замовчуванням
    double mn = 0;
    double mx = 100;
    double st = 1;           // якщо не задано — не писатимемо st=
    const char* pl = nullptr;  // placeholder (опційно)

    // Розбираємо варіативні теги: minVal/maxVal/step/placeholder
    parseSliderArgs(mn, mx, st, pl, sliderArgs...);

    JsonObject field = fields.createNestedObject();

    // Значення. Якщо в твоїй фронт-схемі number-поля ви шлете рядком — лишай як String(value),
    // якщо числом — віддай як число. Нижче — віддамо як число:
    field[key] = value;

    // Будуємо о-рядок
    String oValue = String(toString(FieldType::SLIDER)) + ";" + toString(accessFlag);
    oValue += ";mn=";
    oValue += mn;
    oValue += ";mx=";
    oValue += mx;

    if (!std::isnan(st)) {  // писати лише коли явно задано
      oValue += ";st=";
      oValue += st;
    }
    if (pl && pl[0] != '\0') {  // опційний placeholder
      oValue += ";pl=";
      oValue += pl;
    }

    field["o"] = oValue;
    return field;
  }

  // ---------- CHECKBOX ----------
  static JsonObject addCheckboxField(JsonArray& fields, const char* key, AF accessFlag, bool value) {
    JsonObject field = fields.createNestedObject();
    field[key] = value ? "1" : "0";
    setBasicOptions(field, FieldType::CHECKBOX, accessFlag);
    return field;
  }

  // ---------- SWITCH ----------
  static JsonObject addSwitchField(JsonArray& fields, const char* key, AF accessFlag, bool value) {
    JsonObject field = fields.createNestedObject();
    field[key] = value ? "1" : "0";
    setBasicOptions(field, FieldType::SWITCH, accessFlag);
    return field;
  }

  // ---------- DROPDOWN ----------
  template <typename... Args>
  static JsonObject addDropdownField(JsonArray& fields,
                                     const char* key,
                                     AF accessFlag,
                                     int selectedValue,
                                     Args... optArgs) {
    JsonObject field = fields.createNestedObject();
    field[key] = selectedValue;

    std::vector<Opt> options;
    parseOptionsArgs(options, optArgs...);

    String oValue = String(toString(FieldType::DROPDOWN)) + ";" + toString(accessFlag);
    if (!options.empty()) {
      oValue += ";options=";
      bool first = true;
      for (auto& op : options) {
        if (!first)
          oValue += ",";
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
  static JsonObject addRadioField(JsonArray& fields,
                                  const char* key,
                                  AF accessFlag,
                                  int selectedValue,
                                  Args... optArgs) {
    JsonObject field = fields.createNestedObject();
    field[key] = selectedValue;

    std::vector<Opt> options;
    parseOptionsArgs(options, optArgs...);

    String oValue = String(toString(FieldType::RADIO)) + ";" + toString(accessFlag);
    if (!options.empty()) {
      oValue += ";options=";
      bool first = true;
      for (auto& op : options) {
        if (!first)
          oValue += ",";
        oValue += op.label;
        first = false;
      }
    }

    field["o"] = oValue;
    return field;
  }

  // ---------- TREND (оновлений) ----------
  template <typename... Args>
  static JsonObject addTrendField(JsonArray& fields, const char* key, AF accessFlag, Args... trendArgs) {
    JsonObject field = fields.createNestedObject();
    // Створюємо порожній об'єкт TREND
    JsonObject trendObj = field.createNestedObject(key);

    // Значення за замовчуванням
    String xAxisKeyStr = "timestamp";
    String linesStr;
    bool showLegend = false;
    bool showTooltip = false;
    int maxPts = 100;
    String modeStr = "lineChart";  // Замість "simpledata"

    // Парсимо варіативні аргументи (xAxis(...), lines(...), legend(...), tooltip(...), trendMaxPoints(...), mode(...))
    parseTrendArgs(xAxisKeyStr, linesStr, showLegend, showTooltip, maxPts, modeStr, trendArgs...);

    // Формуємо опцію
    // Наприклад:
    // "trend;rw;mode=lineChart;xAxis=timestamp;lines=key1:hidden=true,color=#8884d8,type=monotone;legend=true;tooltip=true;maxPoints=120"
    String oValue = String(toString(FieldType::TREND)) + ";" + toString(accessFlag);

    // mode=...
    oValue += ";mode=";
    oValue += modeStr;

    // xAxis=...
    if (modeStr == "barChart") {  // Відповідно до нового режиму
      oValue += ";xAxis=keys";
    } else {
      oValue += ";xAxis=";
      oValue += xAxisKeyStr;
    }

    // lines=...
    if (linesStr.length() > 0) {
      oValue += ";lines=";
      oValue += linesStr;
    }

    if (showLegend) {
      oValue += ";legend=true";
    }
    if (showTooltip) {
      oValue += ";tooltip=true";
    }
    if (maxPts > 0) {
      oValue += ";maxPoints=";
      oValue += maxPts;
    }

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
