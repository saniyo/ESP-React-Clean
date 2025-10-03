import React, { useState, useEffect, useRef } from 'react';
import { 
  ListItem, 
  TextField as MuiTextField, 
  Tooltip, 
  Avatar, 
  Divider, 
  ListItemAvatar, 
  ListItemText 
} from '@mui/material';
import { Field } from '../types';
import { styled, useTheme } from '@mui/material/styles';
import { useFieldParser } from '../utils/useFieldParser';
import UpdateIcon from '@mui/icons-material/Update'; // Можна змінити на іншу іконку за потреби

interface TextFieldProps {
  field: Field;
  value: string;
  onChange: (label: string, value: string) => void;
  onBlur?: (label: string, value: string) => void;
}

// Стилізований компонент для аватарки з круглою формою та синім фоном
const StyledAvatar = styled(Avatar)(({ theme }) => ({
  backgroundColor: theme.palette.primary.main,
  borderRadius: '50%', // Забезпечуємо круглу форму
}));

// Стилізований компонент для текстового поля
const StyledTextField = styled(MuiTextField)<{ isReadOnly: boolean }>(({ isReadOnly, theme }) => ({
  cursor: isReadOnly ? 'not-allowed' : 'text',
  pointerEvents: isReadOnly ? 'none' : 'auto',
  '&.Mui-disabled': {
    backgroundColor: theme.palette.action.disabledBackground,
  },
}));

const TextField: React.FC<TextFieldProps> = ({ field, value, onChange, onBlur }) => {
  const { readableLabel, optionMap } = useFieldParser(field.label, field.o || '');
  const [internalValue, setInternalValue] = useState(value);
  const [isEditing, setIsEditing] = useState(false);
  const inputRef = useRef<HTMLInputElement>(null); // Ref для зняття фокусу
  const theme = useTheme();

  const isReadOnly = !!optionMap.r;

  useEffect(() => {
    if (!isEditing) {
      setInternalValue(value);
    }
  }, [value, isEditing]);

  const handleInputChange = (e: React.ChangeEvent<HTMLInputElement>) => {
    const newValue = e.target.value;
    setInternalValue(newValue);
    // Якщо потрібно відправляти зміни під час введення, розкоментуйте наступний рядок
    // onChange(field.label, newValue);
  };

  const handleBlur = () => {
    setIsEditing(false);
    onBlur?.(field.label, internalValue); // Відправляємо зміну при втраті фокусу
    if (onChange) {
      onChange(field.label, internalValue);
    }
  };

  const handleFocus = () => {
    setIsEditing(true);
  };

  const handleKeyDown = (e: React.KeyboardEvent<HTMLInputElement>) => {
    if (e.key === 'Enter') {
      e.preventDefault();
      if (onBlur) {
        onBlur(field.label, internalValue); // Відправляємо зміну при натисканні Enter
      }
      if (onChange) {
        onChange(field.label, internalValue);
      }
      inputRef.current?.blur(); // Знімаємо фокус з поля
    }
  };

  const getLabel = () => (
    <>
      {readableLabel} {isEditing && `: ${value}`}
    </>
  );

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
      <Tooltip title={field.placeholder || 'Enter text'}>
        <StyledTextField
          label={getLabel()}
          placeholder={field.placeholder || ''}
          value={internalValue}
          onChange={handleInputChange}
          onBlur={handleBlur}
          onFocus={handleFocus}
          onKeyDown={handleKeyDown}
          inputRef={inputRef} // Ref для зняття фокусу
          fullWidth
          InputProps={{ readOnly: isReadOnly }}
          disabled={isReadOnly}
          isReadOnly={isReadOnly}
        />
      </Tooltip>
    </ListItem>
  );
};

export default TextField;
