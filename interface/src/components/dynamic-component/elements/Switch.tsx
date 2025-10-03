import React, { useEffect, useState, useRef } from 'react';
import { Switch as MuiSwitch, FormControlLabel, ListItem, Tooltip } from '@mui/material';
import { Field } from '../types';
import { useFieldParser } from '../utils/useFieldParser';

interface SwitchProps {
  field: Field;
  value: any;
  onChange: (label: string, value: any) => void;
  onBlur: (label: string, value: any) => void;
}

const Switch: React.FC<SwitchProps> = ({ field, value, onChange, onBlur }) => {
  const { readableLabel, optionMap } = useFieldParser(field.label, field.o || '');
  const isReadOnly = !!optionMap.r; // Перевірка на "read-only"

  const [localValue, setLocalValue] = useState(!!value);
  const isInteracting = useRef(false);

  useEffect(() => {
    if (!isInteracting.current) {
      setLocalValue(!!value); // Оновлення стану, якщо не взаємодіємо
    }
  }, [value]);

  const handleClick = () => {
    if (isReadOnly) return; // Забороняємо взаємодію, якщо компонент "read-only"
    const newChecked = !localValue;
    setLocalValue(newChecked);
    isInteracting.current = true;
    const numericValue = newChecked ? 1 : 0;
    onChange(field.label, numericValue);
    onBlur(field.label, numericValue);
  };

  const handleMouseLeave = () => {
    isInteracting.current = false; // Завершення взаємодії
  };

  return (
    <ListItem onMouseLeave={handleMouseLeave}>
      <Tooltip title={isReadOnly ? 'Read-only field' : 'Click to toggle value'}>
        <FormControlLabel
          control={
            <MuiSwitch
              checked={localValue}
              onClick={handleClick}
              disabled={isReadOnly} // Стан "read-only"
            />
          }
          label={readableLabel}
        />
      </Tooltip>
    </ListItem>
  );
};

export default Switch;
