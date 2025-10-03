// NumberField.tsx (покращений)
import React, { useState, useEffect } from 'react';
import {
  ListItem,
  TextField as MuiTextField,
  Tooltip,
  Avatar,
  Divider,
  ListItemAvatar,
  ListItemText,
} from '@mui/material';
import { Field } from '../types';
import { styled, useTheme } from '@mui/material/styles';
import { useFieldParser } from '../utils/useFieldParser';
import UpdateIcon from '@mui/icons-material/Update';

interface NumberFieldProps {
  field: Field;
  value: string; // значення, яке приходить зверху
  onChange: (label: string, value: string) => void;
  onBlur?: (label: string, value: string) => void;
}

const StyledAvatar = styled(Avatar)(({ theme }) => ({
  backgroundColor: theme.palette.primary.main,
  borderRadius: '50%',
}));

const NumberField: React.FC<NumberFieldProps> = ({
  field,
  value,
  onBlur,
  onChange,
}) => {
  const theme = useTheme();
  const { readableLabel, optionMap } = useFieldParser(field.label, field.o || '');
  const isReadOnly = !!optionMap.r;
  const minValue = optionMap.mn?.value;
  const maxValue = optionMap.mx?.value;

  // Локальний стан (якщо треба редагувати «на льоту»)
  const [internalValue, setInternalValue] = useState(value);
  const [isEditing, setIsEditing] = useState(false);

  // Якщо зовнішнє value змінилося (зверху), оновлюємо локальний стан,
  // але лише якщо користувач зараз не редагує
  useEffect(() => {
    if (!isEditing) {
      setInternalValue(value);
    }
  }, [value, isEditing]);

  // Коли користувач вводить
  const handleInputChange = (e: React.ChangeEvent<HTMLInputElement>) => {
    let newValue = e.target.value;
    // Перевірка меж
    if (minValue !== undefined && +newValue < +minValue) newValue = String(minValue);
    if (maxValue !== undefined && +newValue > +maxValue) newValue = String(maxValue);

    setInternalValue(newValue);
  };

  // Коли користувач втрачає фокус
  const handleBlur = () => {
    setIsEditing(false);
    // -- Викликаємо onBlur, якщо воно передане
    //    і передаємо «фінальне» internalValue
    if (onBlur) {
      onBlur(field.label, internalValue);
    }
    // **Важливо**: НЕ скидаємо internalValue назад на старе value!
    //    Інакше ми втрачатимемо зміни
    // setInternalValue(value); // <-- Прибрати!

    // Також викликаємо onChange, щоб «сказати» вищому компоненту,
    // що внутрішній стан змінився. (Якщо це потрібно)
    if (onChange) {
      onChange(field.label, internalValue);
    }
  };

  // Коли поле отримує фокус
  const handleFocus = () => {
    setIsEditing(true);
  };

  // Якщо користувач натискає Enter
  const handleKeyDown = (e: React.KeyboardEvent<HTMLInputElement>) => {
    if (e.key === 'Enter') {
      e.preventDefault();
      handleBlur(); 
    }
  };

  if (isReadOnly) {
    return (
      <>
        <ListItem>
          <ListItemAvatar>
            <StyledAvatar>
              <UpdateIcon sx={{ color: theme.palette.common.white }} />
            </StyledAvatar>
          </ListItemAvatar>
          <ListItemText primary={readableLabel} secondary={value} />
        </ListItem>
        <Divider variant="inset" component="li" />
      </>
    );
  }

  return (
    <ListItem>
      <Tooltip title={field.placeholder || 'Enter a value'}>
        <StyledTextField
          type="number"
          label={readableLabel}
          placeholder={field.placeholder || ''}
          value={internalValue}
          onChange={handleInputChange}
          onFocus={handleFocus}
          onBlur={handleBlur}
          onKeyDown={handleKeyDown}
          fullWidth
        />
      </Tooltip>
    </ListItem>
  );
};

const StyledTextField = styled(MuiTextField)(() => ({
  // за потреби кастомні стилі
}));

export default NumberField;
