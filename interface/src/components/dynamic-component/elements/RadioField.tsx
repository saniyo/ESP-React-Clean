import React, { useEffect, useState } from 'react';
import {
  FormControl,
  FormControlLabel,
  RadioGroup,
  Radio,
  ListItem,
  Tooltip,
  ListItemAvatar,
  Avatar,
  ListItemText,
  Divider,
  FormLabel
} from '@mui/material';
import { Field } from '../types';
import { styled, useTheme } from '@mui/material/styles';
import { useFieldParser } from '../utils/useFieldParser';
import UpdateIcon from '@mui/icons-material/Update'; // Імпортуємо UpdateIcon

interface RadioFieldProps {
  field: Field;
  value: number; // Приймає числовий індекс обраного значення
  onChange: (label: string, value: number) => void;
  onBlur?: (label: string, value: number) => void;
}

// Стилізований компонент для аватарки з круглою формою та синім фоном
const StyledAvatar = styled(Avatar)(({ theme }) => ({
  backgroundColor: theme.palette.primary.main,
  borderRadius: '50%', // Забезпечуємо круглу форму
  width: theme.spacing(5), // Встановлюємо розміри аватарки
  height: theme.spacing(5),
}));

const RadioField: React.FC<RadioFieldProps> = ({ field, value, onChange, onBlur }) => {
  const { readableLabel, optionMap } = useFieldParser(field.label, field.o || '');
  const theme = useTheme();

  const isReadOnly = !!optionMap.r;

  // Отримання опцій з optionMap
  const optionsVal = optionMap.options?.value;
  const radioOptions = Array.isArray(optionsVal)
    ? (optionsVal as string[])
    : [];

  const [internalValue, setInternalValue] = useState<number>(value);

  useEffect(() => {
    setInternalValue(value);
  }, [value]);

  const handleChange = (event: React.ChangeEvent<HTMLInputElement>) => {
    const selectedValue = event.target.value;
    const selectedIndex = radioOptions.indexOf(selectedValue);
    if (selectedIndex !== -1) {
      const newValue = selectedIndex + 1;
      setInternalValue(newValue);
      onChange(field.label, newValue);
      onBlur?.(field.label, newValue);
    }
  };

  const hint = `${Object.keys(optionMap).join(', ')}` || 'This is a radio field';

  if (isReadOnly) {
    return (
      <>
        <ListItem>
          <ListItemAvatar>
            <StyledAvatar>
              <UpdateIcon sx={{ color: theme.palette.common.white }} aria-label="read-only icon" />
            </StyledAvatar>
          </ListItemAvatar>
          <ListItemText
            primary={readableLabel}
            secondary={radioOptions[internalValue - 1] || ''}
          />
        </ListItem>
        <Divider variant="inset" component="li" />
      </>
    );
  }

  return (
    <ListItem>
      <FormControl component="fieldset">
        <FormLabel component="legend">{readableLabel}</FormLabel>
        <Tooltip title={hint}>
          <RadioGroup
            value={radioOptions[internalValue - 1] || ''}
            onChange={handleChange}
          >
            {radioOptions.map((option: string, index: number) => (
              <FormControlLabel
                key={index}
                value={option}
                control={<Radio />}
                label={option}
              />
            ))}
          </RadioGroup>
        </Tooltip>
      </FormControl>
    </ListItem>
  );
};

export default RadioField;
