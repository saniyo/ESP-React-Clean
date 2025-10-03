import React, { useEffect, useState } from 'react';
import {
  FormControl,
  InputLabel,
  MenuItem,
  Select,
  ListItem,
  Tooltip,
  ListItemAvatar,
  Avatar,
  ListItemText,
  Divider,
  SelectChangeEvent
} from '@mui/material';
import { Field } from '../types';
import { styled, useTheme } from '@mui/material/styles';
import { useFieldParser } from '../utils/useFieldParser';
import UpdateIcon from '@mui/icons-material/Update'; // Імпортуємо UpdateIcon

interface DropdownProps {
  field: Field;
  value: string | number;
  onChange: (label: string, value: number) => void;
  onBlur?: (label: string, value: number) => void;
}

// Стилізований компонент для аватарки з круглою формою та синім фоном
const StyledAvatar = styled(Avatar)(({ theme }) => ({
  backgroundColor: theme.palette.primary.main,
  borderRadius: '50%', // Забезпечуємо круглу форму
  width: theme.spacing(5), // Наприклад, 32px
  height: theme.spacing(5),
}));

// Стилізований компонент для Select з використанням transient props ($isReadOnly)
const StyledSelect = styled((props: any) => <Select {...props} />)<{ $isReadOnly: boolean }>(({ $isReadOnly }) => ({
  cursor: $isReadOnly ? 'not-allowed' : 'pointer',
  pointerEvents: $isReadOnly ? 'none' : 'auto',
  '& .MuiOutlinedInput-notchedOutline': {
    borderColor: $isReadOnly ? 'transparent' : undefined,
  },
}));

const Dropdown: React.FC<DropdownProps> = ({ field, value, onChange, onBlur }) => {
  const { readableLabel, optionMap } = useFieldParser(field.label, field.o || '');
  const theme = useTheme();

  const isReadOnly = !!optionMap.r;

  // Отримання опцій з optionMap
  const optionsVal = optionMap.options?.value;
  const dropdownOptions = Array.isArray(optionsVal)
    ? (optionsVal as string[])
    : [];

  // Визначаємо індекс та значення
  const [internalValue, setInternalValue] = useState<string>(
    dropdownOptions[(value as number) - 1] || ''
  );

  useEffect(() => {
    setInternalValue(dropdownOptions[(value as number) - 1] || '');
  }, [value, dropdownOptions]);

  const handleChange = (event: SelectChangeEvent<string>, child: React.ReactNode) => {
    const selectedValue = event.target.value;
    const selectedIndex = dropdownOptions.indexOf(selectedValue);
    if (selectedIndex !== -1) {
      const newValue = selectedIndex + 1;
      setInternalValue(selectedValue);
      onChange(field.label, newValue);
      onBlur?.(field.label, newValue);
    }
  };

  const hint = `${Object.keys(optionMap).join(', ')}` || 'This is a dropdown field';

  if (isReadOnly) {
    return (
      <>
        <ListItem>
          <ListItemAvatar>
            <StyledAvatar>
              <UpdateIcon sx={{ color: theme.palette.common.white }} />
            </StyledAvatar>
          </ListItemAvatar>
          <ListItemText primary={readableLabel} secondary={internalValue} />
        </ListItem>
        <Divider variant="inset" component="li" />
      </>
    );
  }

  return (
    <ListItem>
      <Tooltip title={hint}>
        <FormControl fullWidth variant="outlined">
          <InputLabel>{readableLabel}</InputLabel>
          <StyledSelect
            value={internalValue}
            onChange={handleChange}
            label={readableLabel}
            $isReadOnly={isReadOnly}
          >
            {dropdownOptions.map((option: string, index: number) => (
              <MenuItem key={index} value={option}>
                {option}
              </MenuItem>
            ))}
          </StyledSelect>
        </FormControl>
      </Tooltip>
    </ListItem>
  );
};

export default Dropdown;
