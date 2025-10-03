// src/utils/liveCash.ts
type Label = string;
export type Source = 'ws' | 'rest';

type Listener = (label: Label, value: any, source: Source, ts: number) => void;

interface Entry {
  value: any;
  ts: number;
  source: Source;
}

const store = new Map<Label, Entry>();
const listeners = new Set<Listener>();

// повертає голе значення (як і раніше)
export function getLiveValue(label: Label): any | undefined {
  return store.has(label) ? store.get(label)!.value : undefined;
}

export function getLiveSnapshot(labels: Label[]): Record<string, any> {
  const out: Record<string, any> = {};
  labels.forEach((l) => {
    if (store.has(l)) out[l] = store.get(l)!.value;
  });
  return out;
}

// ОНОВЛЕННЯ: пишемо з джерелом і timestamp
export function setLiveValue(label: Label, value: any, source: Source = 'ws'): void {
  const now = Date.now();
  const prev = store.get(label);
  if (!prev || now >= prev.ts) {
    store.set(label, { value, ts: now, source });
    listeners.forEach((l) => l(label, value, source, now));
  }
}

// bulk-оновлення (зручно для SAVE)
export function setLiveValuesBulk(patch: Record<string, any>, source: Source = 'rest'): void {
  const now = Date.now();
  Object.keys(patch).forEach((label, i) => {
    const ts = now + i; // гарантуємо зростаючий ts у межах однієї операції
    const prev = store.get(label);
    if (!prev || ts >= prev.ts) {
      const value = patch[label];
      store.set(label, { value, ts, source });
      listeners.forEach((l) => l(label, value, source, ts));
    }
  });
}

export function subscribeLiveValues(fn: Listener): () => void {
  listeners.add(fn);
  return () => { listeners.delete(fn); };
}
