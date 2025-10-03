export const fieldConfigurations = {
  text:     { options: ['r', 'rw', 'pl'] as const },
  number:   { options: ['r', 'rw', 'mn', 'mx', 'f', 'pl'] as const },
  // ДОДАНО: окремий тип slider (не ламає number)
  slider:   { options: ['r', 'rw', 'mn', 'mx', 'pl', 'st'] as const },
  checkbox: { options: ['r', 'rw', 'pl'] as const },
  button:   { options: ['r', 'rw', 'pl'] as const },
  switch:   { options: ['r', 'rw', 'pl'] as const },
  dropdown: { options: ['r', 'rw', 'options', 'pl'] as const },
  textarea: { options: ['r', 'rw', 'pl'] as const },
  radio:    { options: ['r', 'rw', 'options', 'pl'] as const },
  trend:    { options: ['to', 'xAxis', 'lines', 'legend', 'tooltip', 'maxPoints', 'mode'] as const }
};

export type FieldType = keyof typeof fieldConfigurations;
export type FieldOptionKey = typeof fieldConfigurations[FieldType]['options'][number];

export interface ParsedFieldOption {
  key: FieldOptionKey;
  value: string | boolean | string[] | number | undefined;
}

export interface ParsedFieldOptions {
  readableLabel: string;
  optionMap: Partial<Record<FieldOptionKey, ParsedFieldOption>>;
  finalType: FieldType;
}

/**
 * Хелпер: повертає значення після 'key=' або після 'key' (обидва формати підтримуються).
 * Напр., getAfter('mn=10.00','mn') -> '10.00'; getAfter('plHint','pl') -> 'Hint'
 */
function getAfter(opt: string, key: string): string {
  if (opt.startsWith(key + '=')) return opt.slice((key + '=').length);
  if (opt.startsWith(key))       return opt.slice(key.length);
  return '';
}

/** Парсери опцій (виправлено з урахуванням '=') */
const optionParsers: Record<string, (option: string) => ParsedFieldOption> = {
  r:  () => ({ key: 'r',  value: true }),
  rw: () => ({ key: 'rw', value: false }),

  // ВИПРАВЛЕНО: було slice(2) → давало '=10.00' => NaN
  mn: (opt) => {
    const raw = getAfter(opt, 'mn');
    const n   = parseFloat(raw);
    return { key: 'mn', value: Number.isFinite(n) ? n : undefined };
  },

  // ВИПРАВЛЕНО: було slice(2)
  mx: (opt) => {
    const raw = getAfter(opt, 'mx');
    const n   = parseFloat(raw);
    return { key: 'mx', value: Number.isFinite(n) ? n : undefined };
  },

  // ВИПРАВЛЕНО: бек шле f=..., раніше було slice(1)
  f:  (opt) => {
    const raw = getAfter(opt, 'f');
    const n   = parseFloat(raw);
    return { key: 'f', value: Number.isFinite(n) ? n : undefined };
  },

  // ВИПРАВЛЕНО: бек шле pl=..., раніше було slice(2)
  pl: (opt) => {
    const val = getAfter(opt, 'pl').trim();
    return { key: 'pl', value: val };
  },

  // ДОДАНО: step для slider (і можна юзати з number)
  st: (opt) => {
    const raw = getAfter(opt, 'st');
    const n   = parseFloat(raw);
    return { key: 'st', value: Number.isFinite(n) ? n : undefined };
  },

  to:      (opt) => ({ key: 'to',      value: getAfter(opt, 'to') }),
  legend:  ()   => ({ key: 'legend',  value: true }),
  tooltip: ()   => ({ key: 'tooltip', value: true }),

  // lines=key1:color=...,key2:color=...
  lines: (opt) => {
    const val = getAfter(opt, 'lines');
    return { key: 'lines', value: val };
  },

  // options=Option1,Option2
  options: (opt) => {
    const val = getAfter(opt, 'options');
    const items = val.split(',').map(s => s.trim()).filter(Boolean);
    return { key: 'options', value: items };
  },

  maxPoints: (opt) => {
    const raw = getAfter(opt, 'maxPoints');
    const n   = parseInt(raw, 10);
    return { key: 'maxPoints', value: Number.isFinite(n) ? n : undefined };
  },

  mode: (opt) => {
    const val = getAfter(opt, 'mode').trim();
    return { key: 'mode', value: val };
  }
};

// Всі відомі ключі
const allKnownKeys = Array.from(
  new Set(
    Object.values(fieldConfigurations)
      .flatMap(c => c.options)
      .concat(Object.keys(optionParsers) as FieldOptionKey[])
  )
).sort((a, b) => b.length - a.length);

/**
 * parseFieldOptions(label, optionsString):
 * - Розбиває optionsString по ';'
 * - Зшиває segments для "lines=" (особливо, якщо line-конфіги розкидані через ";")
 * - Викликає optionParsers[..] для кожного сегмента
 * - finalType => 'trend','radio','dropdown','text','slider',...
 */
export function parseFieldOptions(label: string, optionsString: string): ParsedFieldOptions {
  const readableLabel = label
    .replace(/_/g, ' ')
    .replace(/(?:^|\s)\S/g, m => m.toUpperCase());

  if (!optionsString) {
    return {
      readableLabel,
      optionMap: {},
      finalType: 'text'
    };
  }

  let splitted = optionsString.split(';').map(s => s.trim()).filter(Boolean);

  // Визначення типу (враховуючи доданий 'slider')
  const supportedTypes = Object.keys(fieldConfigurations) as FieldType[];
  const foundType = splitted.find((opt) => supportedTypes.includes(opt as FieldType)) as FieldType | undefined;
  const finalType = foundType || 'text';

  const config = fieldConfigurations[finalType];

  // Злипаємо "lines="
  const merged: string[] = [];
  for (let i = 0; i < splitted.length; i++) {
    const seg = splitted[i];
    if (seg.startsWith('lines=')) {
      let linesFull = seg;
      while (i + 1 < splitted.length) {
        const nextSeg = splitted[i + 1];
        const knownPrefix = allKnownKeys.some(k => nextSeg.startsWith(k + '=') || nextSeg === k);
        if (!knownPrefix && nextSeg.includes(':')) {
          linesFull += ';' + nextSeg;
          i++;
        } else {
          break;
        }
      }
      merged.push(linesFull);
    } else {
      merged.push(seg);
    }
  }

  const optionMap: Partial<Record<FieldOptionKey, ParsedFieldOption>> = {};

  merged.forEach(option => {
    const prefix = allKnownKeys.find(k => option === k || option.startsWith(`${k}=`));
    if (!prefix) return;

    if (optionParsers[prefix]) {
      const parsed = optionParsers[prefix](option);
      // Перевіряємо відповідність дозволеним опціям для даного типу
      if ((config.options as readonly string[]).includes(parsed.key)) {
        optionMap[parsed.key as FieldOptionKey] = parsed;
      }
    } else if ((config.options as readonly string[]).includes(prefix as FieldOptionKey)) {
      // Ключ без '=true', наприклад "legend"
      optionMap[prefix as FieldOptionKey] = { key: prefix as FieldOptionKey, value: true };
    }
  });

  return {
    readableLabel,
    optionMap,
    finalType
  };
}
