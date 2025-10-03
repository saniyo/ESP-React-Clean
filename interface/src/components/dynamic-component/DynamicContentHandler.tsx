import React, { FC, useEffect, useState } from 'react';
import { List, Typography } from '@mui/material';
import { Field } from './types';
import TextField from './elements/TextField';
import NumberField from './elements/NumberField';
import SliderField from './elements/SliderField';
import Checkbox from './elements/Checkbox';
import Switch from './elements/Switch';
import Dropdown from './elements/Dropdown';
import Textarea from './elements/TextArea';
import RadioField from './elements/RadioField';
import TrendChart from './elements/TrendChart';
import DefaultComponent from './elements/DefaultComponent';

interface DynamicContentHandlerProps {
  title: string;
  description: string;
  fields: Field[];
  onInputChange?: (label: string, value: any) => void;
  onFieldBlur?: (label: string, value: any) => void;
  onFieldFocus?: (label: string) => void;
  onSubmit?: (event: React.FormEvent) => void;
  hideTitle?: boolean;
}

const DynamicContentHandler: FC<DynamicContentHandlerProps> = ({
  title,
  description,
  fields,
  onInputChange,
  onFieldBlur,
  onSubmit,
  hideTitle = false,
}) => {
  const [formState, setFormState] = useState<Record<string, any>>({});

  useEffect(() => {
    const initialState: Record<string, any> = {};
    fields.forEach((field) => {
      initialState[field.label] = field.value;
    });
    setFormState(initialState);
  }, [fields]);

  const handleInputChange = (label: string, value: any) => {
    setFormState((prevState) => ({
      ...prevState,
      [label]: value,
    }));
    onInputChange && onInputChange(label, value);
  };

  const handleFieldBlur = (label: string, value: any) => {
    setFormState((prevState) => ({
      ...prevState,
      [label]: value,
    }));
    onFieldBlur && onFieldBlur(label, value);
  };

  const renderField = (field: Field, index: number): JSX.Element => {
    const fieldKey = `${field.label}-${index}`;
    const commonProps = {
      field,
      value: formState[field.label],
      onChange: handleInputChange,
      onBlur: (lbl: string, val: any) => handleFieldBlur(lbl, val),
    };

    switch (field.type) {
      case 'text':
        return <TextField key={fieldKey} {...commonProps} />;
      case 'number':
        return <NumberField key={fieldKey} {...commonProps} />;
      case 'slider':
        return <SliderField key={fieldKey} {...commonProps} />;
      case 'checkbox':
        return <Checkbox key={fieldKey} {...commonProps} />;
      case 'switch':
        return <Switch key={fieldKey} {...commonProps} />;
      case 'dropdown':
        return <Dropdown key={fieldKey} {...commonProps} />;
      case 'textarea':
        return <Textarea key={fieldKey} {...commonProps} />;
      case 'radio':
        return <RadioField key={fieldKey} {...commonProps} />;
      case 'trend':
        return <TrendChart key={fieldKey} {...commonProps} />;
      default:
        return <DefaultComponent key={fieldKey} field={field} />;
    }
  };

  return (
    <form onSubmit={onSubmit}>
      {!hideTitle && (
        <Typography variant="h4" gutterBottom>
          {title}
        </Typography>
      )}
      <Typography variant="body1" gutterBottom>
        {description}
      </Typography>
      <List>
        {fields.map((field, index) => renderField(field, index))}
      </List>
    </form>
  );
};

export default DynamicContentHandler;
