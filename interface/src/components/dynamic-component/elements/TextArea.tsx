import React, { useState, useEffect } from 'react';
import { 
  TextField as MuiTextField, 
  Tooltip, 
  ListItem, 
  ListItemAvatar, 
  Avatar, 
  ListItemText, 
  Divider, 
  Typography 
} from '@mui/material';
import { Field } from '../types';
import { styled, useTheme } from '@mui/material/styles';
import { useFieldParser } from '../utils/useFieldParser';
import UpdateIcon from '@mui/icons-material/Update'; // Імпортуємо UpdateIcon

interface TextareaProps {
  field: Field;
  value: string;
  onChange: (label: string, value: string) => void;
  onBlur?: (label: string, value: string) => void;
}

// Стилізований компонент для аватарки з круглою формою та синім фоном
const StyledAvatar = styled(Avatar)(({ theme }) => ({
  backgroundColor: theme.palette.primary.main,
  borderRadius: '50%',
  width: theme.spacing(5),
  height: theme.spacing(5),
}));

// Стилізований компонент для текстового поля у режимі тільки для читання
const StyledReadOnlyTextField = styled(MuiTextField)(({ theme }) => ({
  '& .MuiInputBase-root.Mui-disabled': {
    backgroundColor: theme.palette.action.disabledBackground,
  },
}));

const Textarea: React.FC<TextareaProps> = ({ field, value, onChange, onBlur }) => {
  const { readableLabel, optionMap } = useFieldParser(field.label, field.o || '');
  const theme = useTheme();
  const isReadOnly = !!optionMap.r;

  const [internalValue, setInternalValue] = useState<string>(value);
  const [isEditing, setIsEditing] = useState(false);

  useEffect(() => {
    if (!isEditing) {
      setInternalValue(value);
    }
  }, [value, isEditing]);

  const handleInputChange = (e: React.ChangeEvent<HTMLInputElement | HTMLTextAreaElement>) => {
    const newValue = e.target.value;
    setInternalValue(newValue);
    onChange(field.label, newValue); // Відправляємо зміну під час введення
  };

  const handleBlur = () => {
    setIsEditing(false);
    onBlur?.(field.label, internalValue); // Відправляємо зміну при втраті фокусу
  };

  const handleFocus = () => {
    setIsEditing(true);
  };

  const hint = field.placeholder || 'Enter text';

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
          />
        </ListItem>
        <ListItem>
          <StyledReadOnlyTextField
            variant="outlined"
            multiline
            minRows={3}
            fullWidth
            value={internalValue}
            disabled
            InputProps={{
              readOnly: true,
            }}
          />
        </ListItem>
        <Divider variant="inset" component="li" />
      </>
    );
  }

  return (
    <ListItem>
      <Tooltip title={hint}>
        <MuiTextField
          label={readableLabel}
          placeholder={field.placeholder || ''}
          value={internalValue}
          onChange={handleInputChange}
          onBlur={handleBlur}
          onFocus={handleFocus}
          multiline
          minRows={3}
          fullWidth
        />
      </Tooltip>
    </ListItem>
  );
};

export default Textarea;
