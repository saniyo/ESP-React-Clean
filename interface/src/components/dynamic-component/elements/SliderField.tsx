import React, { useEffect, useMemo, useState } from 'react';
import {
  ListItem,
  Tooltip,
  Avatar,
  Divider,
  ListItemAvatar,
  ListItemText,
  Typography,
  Box,
} from '@mui/material';
import { styled, useTheme } from '@mui/material/styles';
import Slider from '@mui/material/Slider';
import LinearScaleIcon from '@mui/icons-material/LinearScale';
import UpdateIcon from '@mui/icons-material/Update';
import { Field } from '../types';
import { useFieldParser } from '../utils/useFieldParser';

interface SliderFieldProps {
  field: Field;
  value: number | string | undefined;
  onChange: (label: string, value: number) => void;
  onBlur?: (label: string, value: number) => void;
}

const StyledAvatar = styled(Avatar)(({ theme }) => ({
  backgroundColor: theme.palette.primary.main,
  borderRadius: '50%',
}));

const ValuePill = styled('span')(({ theme }) => ({
  display: 'inline-block',
  padding: '2px 8px',
  borderRadius: 12,
  fontSize: 12,
  marginLeft: theme.spacing(1),
  border: `1px solid ${theme.palette.divider}`,
}));

const SliderField: React.FC<SliderFieldProps> = ({
  field,
  value,
  onChange,
  onBlur,
}) => {
  const theme = useTheme();
  const { readableLabel, optionMap } = useFieldParser(field.label, field.o || '');

  const isReadOnly = !!optionMap.r;

  console.log('optionMap', optionMap);

  // ADDED: спочатку беремо field.min/field.max/field.step, потім optionMap, далі дефолт
  const minValue =
    Number.isFinite(Number((field as any).min))
      ? Number((field as any).min)
      : (Number.isFinite(Number(optionMap.mn?.value)) ? Number(optionMap.mn!.value) : 0);

  const maxValue =
    Number.isFinite(Number((field as any).max))
      ? Number((field as any).max)
      : (Number.isFinite(Number(optionMap.mx?.value)) ? Number(optionMap.mx!.value) : 100);



  const step =
    Number.isFinite(Number((field as any).step)) && Number((field as any).step) > 0
      ? Number((field as any).step)
      : (Number.isFinite(Number(optionMap.st?.value)) && Number(optionMap.st!.value) > 0
        ? Number(optionMap.st!.value)
        : 1);

  console.log('minValue', minValue);
  console.log('maxValue', maxValue);
  console.log('step', step);
  // console.log('optionminValue', optionMap.mn!.value);
  // console.log('optionmaxValue', optionMap.mx!.value);
  // console.log('optionstep', optionMap.st!.value);


  // ADDED: функція для безпечної нормалізації вхідного значення
  const toNum = (v: unknown): number => {
    if (typeof v === 'number') return Number.isFinite(v) ? v : NaN;
    if (typeof v === 'string') {
      const t = v.trim();
      if (!t) return NaN;
      const n = Number(t.replace(',', '.'));
      return Number.isFinite(n) ? n : NaN;
    }
    return NaN;
  };

  const numericValue = toNum(value);
  const clamp = (n: number) => Math.min(maxValue, Math.max(minValue, n));

  const [internalValue, setInternalValue] = useState<number>(
    Number.isFinite(numericValue) ? clamp(numericValue) : minValue
  );

  const [isEditing, setIsEditing] = useState(false);

  useEffect(() => {
    if (!isEditing) {
      const n = toNum(value);
      setInternalValue(Number.isFinite(n) ? clamp(n) : minValue);
    }
  }, [value, isEditing, minValue, maxValue]);

  // ADDED: marks для відображення мін/макс на краях
  const marks = useMemo(() => ([
    { value: minValue, label: String(minValue) },
    { value: maxValue, label: String(maxValue) }
  ]), [minValue, maxValue]);

  const handleChange = (_: Event, newVal: number | number[]) => {
    const v = clamp(Array.isArray(newVal) ? newVal[0] : newVal);
    setInternalValue(v);
    onChange && onChange(field.label, v);
  };

  const handleChangeCommitted = (_: React.SyntheticEvent | Event, newVal: number | number[]) => {
    const v = clamp(Array.isArray(newVal) ? newVal[0] : newVal);
    setIsEditing(false);
    onBlur && onBlur(field.label, v);
  };

  const handlePointerDown = () => setIsEditing(true);

  if (isReadOnly) {
    return (
      <>
        <ListItem sx={{ alignItems: "center" }}>
          <ListItemAvatar>
            <StyledAvatar>
              <UpdateIcon sx={{ color: theme.palette.common.white }} />
            </StyledAvatar>
          </ListItemAvatar>
          <ListItemText
            primary={readableLabel}
            secondary={value}
            sx={{
              flex: "0 0 auto", pr: 2
            }}
          />

          {/* ПРАВИЙ: займає весь залишок */}
          <Box sx={{ flex: 1, minWidth: 0 }}>
            <Box mt={1}>
              <Slider
                sx={{
                  '& .MuiSlider-markLabel': {
                    fontSize: '0.7em', // 30% менше
                    color: 'text.secondary',
                  },
                  '& .MuiSlider-markLabelActive': {
                    fontSize: '0.7em', // те ж саме для активної
                    color: 'text.secondary',
                  },
                  '&.Mui-disabled .MuiSlider-markLabel, &.Mui-disabled .MuiSlider-markLabelActive': {
                    color: (theme) => theme.palette.text.disabled,
                  },
                }}
                value={internalValue}
                min={minValue}
                max={maxValue}
                step={step}
                marks={marks}
                valueLabelDisplay="auto"
                onChange={handleChange}
                onChangeCommitted={handleChangeCommitted}
                aria-label={readableLabel}
                disabled
              />
            </Box>
          </Box>
        </ListItem>

        <Divider variant="inset" component="li" />
      </>
    );
  }

  return (
 <>
        <ListItem sx={{ alignItems: "center" }}>
          <ListItemAvatar>
            <StyledAvatar>
              <UpdateIcon sx={{ color: theme.palette.common.white }} />
            </StyledAvatar>
          </ListItemAvatar>
          <ListItemText
            primary={readableLabel}
            secondary={value}
            sx={{
              flex: "0 0 auto", pr: 2
            }}
          />

          {/* ПРАВИЙ: займає весь залишок */}
          <Box sx={{ flex: 1, minWidth: 0 }}>
            <Box mt={1}>
              <Slider
                sx={{
                  '& .MuiSlider-markLabel': {
                    fontSize: '0.7em', // 30% менше
                    color: 'text.secondary',
                  },
                  '& .MuiSlider-markLabelActive': {
                    fontSize: '0.7em', // те ж саме для активної
                    color: 'text.secondary',
                  },
                  '&.Mui-disabled .MuiSlider-markLabel, &.Mui-disabled .MuiSlider-markLabelActive': {
                    color: (theme) => theme.palette.text.disabled,
                  },
                }}
                value={internalValue}
                min={minValue}
                max={maxValue}
                step={step}
                marks={marks}
                valueLabelDisplay="auto"
                onChange={handleChange}
                onChangeCommitted={handleChangeCommitted}
                aria-label={readableLabel}
              />
            </Box>
          </Box>
        </ListItem>

        <Divider variant="inset" component="li" />
      </>
  );
};

export default SliderField;
