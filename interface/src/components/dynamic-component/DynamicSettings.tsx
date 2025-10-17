// DynamicSettings.tsx
import React, { FC, useState, useEffect, useRef } from 'react';
import DynamicContentHandler from './DynamicContentHandler';
import { SectionContent } from '../../components';
import FormLoader from '../loading/FormLoader';
import SubmitButton from './elements/SubmitButton';
import { Field, Form } from './types';
import { parseFieldOptions } from './utils/fieldParser';
import {
  getLiveValue,
  getLiveSnapshot,
  subscribeLiveValues,
  setLiveValuesBulk,
} from './utils/liveCash';

interface DynamicSettingsProps {
  formName: string;
  data: { description: string; fields: Array<{ [key: string]: any; o: string }> };
  saveData: (updatedData: any) => void;
  saving: boolean;
  setData: (newData: any) => void;
}

const DynamicSettings: FC<DynamicSettingsProps> = ({ formName, data, saveData, saving }) => {
  const [formState, setFormState] = useState<Form>({
    title: formName,
    description: data.description || '',
    fields: [],
  });

  // Тут зберігаємо значення у форматі UI.
  // Для булевих — завжди true/false.
  const [changedFields, setChangedFields] = useState<Record<string, any>>({});

  const labelsRef = useRef<string[]>([]);

  // Підвантаження з REST: усе, що булеве — приводимо до boolean.
  useEffect(() => {
    if (!data || !Array.isArray(data.fields)) return;

    const fields: Field[] = data.fields.map((raw) => {
      const o = (raw as any).o || '';
      const fieldName = Object.keys(raw).find((k) => k !== 'o') || '';
      let fieldValue = (raw as any)[fieldName];

      // Якщо є live—значення, воно пріоритетне
      const live = getLiveValue(fieldName);
      if (live !== undefined) fieldValue = live;

      const { optionMap, finalType } = parseFieldOptions(fieldName, o);

      const isBoolType =
        finalType === 'checkbox' || finalType === 'switch' || finalType === 'button';
      if (isBoolType) fieldValue = !!fieldValue;

      const min = optionMap.mn ? parseFloat(optionMap.mn.value as string) : undefined;
      const max = optionMap.mx ? parseFloat(optionMap.mx.value as string) : undefined;
      // деякі конфіги можуть не мати "st" у типах — читаємо обережно
      const step =
        (optionMap as any).st !== undefined ? Number((optionMap as any).st.value) : undefined;
      const placeholder = optionMap.pl ? (optionMap.pl.value as string) : undefined;

      return {
        label: fieldName,
        value: fieldValue,
        type: finalType,
        readOnly: !!optionMap.r,
        min,
        max,
        step,
        placeholder,
        o,
      };
    });

    labelsRef.current = fields.map((f) => f.label);
    setFormState({ title: formName, description: data.description || '', fields });
    setChangedFields({});
  }, [data, formName]);

  // Одноразово накласти snapshot із кешу (в UI-форматі)
  useEffect(() => {
    if (!formState.fields.length) return;

    const snap = getLiveSnapshot(labelsRef.current);
    if (!Object.keys(snap).length) return;

    let changed = false;
    const nextFields = formState.fields.map((f) => {
      if (snap[f.label] !== undefined && snap[f.label] !== f.value) {
        changed = true;
        return { ...f, value: snap[f.label] };
      }
      return f;
    });

    if (changed) {
      setFormState((prev) => ({ ...prev, fields: nextFields }));
    }
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [formState.fields.length]);

  // Підписка на live-оновлення (значення завжди в UI-форматі)
  useEffect(() => {
    const unsub = subscribeLiveValues((label: string, value: any) => {
      if (!labelsRef.current.includes(label)) return;

      setFormState((prev) => {
        const idx = prev.fields.findIndex((f) => f.label === label);
        if (idx === -1) return prev;

        const oldVal = prev.fields[idx].value;
        if (oldVal === value) return prev;

        const nextFields = [...prev.fields];
        nextFields[idx] = { ...nextFields[idx], value };
        return { ...prev, fields: nextFields };
      });
    });

    return unsub;
  }, []);

  const handleInputChange = (label: string, value: any) => {
    // Усе зберігаємо в UI-форматі: булеві — тільки true/false
    const field = formState.fields.find((f) => f.label === label);
    const isBoolType =
      field && (field.type === 'checkbox' || field.type === 'switch' || field.type === 'button');
    const uiValue = isBoolType ? !!value : value;

    setFormState((prevState) => {
      const updatedFields = prevState.fields.map((f) =>
        f.label === label ? { ...f, value: uiValue } : f
      );
      return { ...prevState, fields: updatedFields };
    });

    setChangedFields((prev) => ({ ...prev, [label]: uiValue }));
  };

  const handleFieldBlur = (label: string, value: any) => {
    // Те саме, що й handleInputChange: фіксуємо UI-значення
    const field = formState.fields.find((f) => f.label === label);
    const isBoolType =
      field && (field.type === 'checkbox' || field.type === 'switch' || field.type === 'button');
    const uiValue = isBoolType ? !!value : value;

    setFormState((prevState) => {
      const updatedFields = prevState.fields.map((f) =>
        f.label === label ? { ...f, value: uiValue } : f
      );
      return { ...prevState, fields: updatedFields };
    });

    setChangedFields((prev) => ({ ...prev, [label]: uiValue }));
  };

  const handleSubmit = (event: React.FormEvent) => {
    event.preventDefault();
    if (Object.keys(changedFields).length === 0) return;

    // 1) Оновлюємо live-кеш UI-значеннями (булеві — true/false)
    setLiveValuesBulk(changedFields, 'rest');

    // 2) Відправляємо як є — без жодних 0/1 перетворень
    saveData(changedFields);
  };

  if (!formState.fields) {
    return <FormLoader message="Waiting for WebSocket connection..." />;
  }

  return (
    <SectionContent title={formState.title || 'Settings'} titleGutter>
      <DynamicContentHandler
        title={formState.title || 'Settings'}
        description={formState.description || ''}
        fields={formState.fields || []}
        onInputChange={handleInputChange}
        onFieldBlur={handleFieldBlur}
        onSubmit={handleSubmit}
        hideTitle
      />
      <SubmitButton onSubmit={handleSubmit} saving={saving} />
    </SectionContent>
  );
};

export default DynamicSettings;
