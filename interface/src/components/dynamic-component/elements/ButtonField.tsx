import React, { useEffect, useRef, useState } from 'react';
import { ListItem, Tooltip, Button } from '@mui/material';
import { Field } from '../types';
import { useFieldParser } from '../utils/useFieldParser';

interface ButtonFieldProps {
  field: Field;
  value: any; // 0/1, '0'/'1', boolean — як приходить із бекенду
  onChange: (label: string, value: any) => void;
  onBlur: (label: string, value: any) => void;
}

const ButtonField: React.FC<ButtonFieldProps> = ({ field, value, onChange, onBlur }) => {
  const { readableLabel, optionMap } = useFieldParser(field.label, field.o || '');
  const isReadOnly = !!optionMap.r;

  // показуємо “увімкнутий/вимкнутий” стан локально, але джерело істини — бекенд
  const toBool = (v: any) => (v === 1 || v === '1' || v === true);
  const [localOn, setLocalOn] = useState<boolean>(toBool(value));
  const interactingRef = useRef(false);

  // синхронізація зверху: якщо не взаємодіємо — приймаємо бекендове значення
  // якщо взаємодіємо і бекенд прислав те ж саме, закінчуємо взаємодію
  useEffect(() => {
    const ext = toBool(value);
    if (!interactingRef.current) {
      setLocalOn(ext);
    } else if (ext === localOn) {
      interactingRef.current = false;
    }
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [value]);

  const labelText = field.placeholder || readableLabel || '';
  const tooltipText = optionMap.pl?.value?.toString() || (isReadOnly ? 'Read-only action' : 'Click to toggle');

  const handleClick = () => {
    if (isReadOnly) return;
    // оптимістично перемикаємо локально, бекенд підтвердить і зафіксує
    const next = !localOn;
    setLocalOn(next);
    interactingRef.current = true;

    const numeric = next ? 1 : 0;
    onChange(field.label, numeric);
    onBlur(field.label, numeric);
  };

  const handleKeyDown: React.KeyboardEventHandler<HTMLButtonElement> = (e) => {
    if (isReadOnly) return;
    if (e.key === 'Enter' || e.key === ' ') {
      e.preventDefault();
      handleClick();
    }
  };

  return (
    <ListItem>
      <Tooltip title={tooltipText}>
        <span>
          <Button
            variant={localOn ? 'contained' : 'outlined'}
            color={localOn ? 'primary' : 'inherit'}
            onClick={handleClick}
            onKeyDown={handleKeyDown}
            disabled={isReadOnly}
          >
            {labelText}
          </Button>
        </span>
      </Tooltip>
    </ListItem>
  );
};

export default ButtonField;
