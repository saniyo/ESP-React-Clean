import React, { FC, useState, useEffect, useRef } from 'react';
import DynamicContentHandler from './DynamicContentHandler';
import { SectionContent } from '../../components';
import FormLoader from '../loading/FormLoader';
import SubmitButton from './elements/SubmitButton';
import { Field, Form } from './types';
import { parseFieldOptions } from './utils/fieldParser';
import { getLiveValue, getLiveSnapshot, subscribeLiveValues, setLiveValuesBulk } from './utils/liveCash';

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

  const [changedFields, setChangedFields] = useState<Record<string, any>>({});

  // зберігаємо актуальний список лейблів, щоб не прив’язувати ефекти до formState
  const labelsRef = useRef<string[]>([]);

  // будуємо fields з REST + поверх кладемо live (якщо є)
  useEffect(() => {
    if (!data || !Array.isArray(data.fields)) return;

    const fields: Field[] = data.fields.map((field) => {
      const o = field.o || '';
      const fieldName = Object.keys(field).find((key) => key !== 'o') || '';
      let fieldValue = field[fieldName];

      // накладаємо live-значення поверх REST
      const live = getLiveValue(fieldName);
      if (live !== undefined) fieldValue = live;

      const { optionMap, finalType } = parseFieldOptions(fieldName, o);
      const isReadOnly = !!optionMap.r;
      const min = optionMap.mn ? parseFloat(optionMap.mn.value as string) : undefined;
      const max = optionMap.mx ? parseFloat(optionMap.mx.value as string) : undefined;
      const placeholder = optionMap.pl ? (optionMap.pl.value as string) : undefined;
      const step = optionMap.st !== undefined ? Number(optionMap.st.value) : undefined;

      if (finalType === 'checkbox' || finalType === 'switch') {
        fieldValue = fieldValue === 1 || fieldValue === '1' || fieldValue === true;
      }

      return {
        label: fieldName,
        value: fieldValue,
        type: finalType,
        readOnly: isReadOnly,
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

  // одразу після маунта/оновлення списку полів накладемо поточний snapshot live-значень
  useEffect(() => {
    if (!formState.fields.length) return;

    const snap = getLiveSnapshot(labelsRef.current);
    if (!Object.keys(snap).length) return;

    // готуємо патч тільки якщо є реальні зміни
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
    // важливо: без залежності від formState, щоб не зациклитись
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [formState.fields.length]);

  // підписка на подальші live-апдейти: апдейтимо тільки, якщо значення дійсно змінилось
  useEffect(() => {
    const unsub = subscribeLiveValues((label: string, value: any) => {
      if (!labelsRef.current.includes(label)) return;

      setFormState(prev => {
        const idx = prev.fields.findIndex(f => f.label === label);
        if (idx === -1) return prev;

        const oldVal = prev.fields[idx].value;
        if (oldVal === value) return prev;

        const nextFields = [...prev.fields];
        nextFields[idx] = { ...nextFields[idx], value };
        return { ...prev, fields: nextFields };
      });
    });

    return unsub; // тепер це () => void, і React задоволений
  }, []);

  const handleInputChange = (label: string, value: any) => {
    setFormState((prevState) => {
      const updatedFields = prevState.fields.map((field) =>
        field.label === label ? { ...field, value } : field
      );
      return { ...prevState, fields: updatedFields };
    });

    setChangedFields((prevChanged) => ({
      ...prevChanged,
      [label]: value,
    }));
  };

  const handleFieldBlur = (label: string, value: any) => {
    const field = formState.fields.find((f) => f.label === label);
    let sendValue = value;
    if (field && (field.type === 'checkbox' || field.type === 'switch')) {
      sendValue = value ? 1 : 0;
    }

    setFormState((prevState) => {
      const updatedFields = prevState.fields.map((fld) =>
        fld.label === label ? { ...fld, value } : fld
      );
      return { ...prevState, fields: updatedFields };
    });

    setChangedFields((prevChanged) => ({
      ...prevChanged,
      [label]: sendValue,
    }));
  };

  const handleSubmit = (event: React.FormEvent) => {
    event.preventDefault();
    if (Object.keys(changedFields).length > 0) {
      // 1) оптимістично оновлюємо live-кеш — щоб одразу побачити свіжі REST-значення
      setLiveValuesBulk(changedFields, 'rest');

      // 2) як і було — шлемо на бек
      saveData(changedFields);
    }
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
