/* DynamicStatus.tsx */

import React, { FC, useEffect, useState, useCallback, useRef } from 'react';
import DynamicContentHandler     from './DynamicContentHandler';
import { SectionContent }        from '../../components';
import FormLoader                from '../loading/FormLoader';
import { Form, Field }           from './types';
import { parseFieldOptions }     from './utils/fieldParser';
import { setLiveValue } from './utils/liveCash';

/* ----------- props ---------- */
interface DynamicStatusProps {
  formName : string;
  data     : { description:string; fields:Array<{[k:string]:any;o:string}> };
  wsData   : any;
  connected: boolean;
  saving   : boolean;
  originId : string;
  setData  : (d:any)=>void;
  disconnect   : ()=>void;
  updateWsData : (d:React.SetStateAction<any>, tx?:boolean, clr?:boolean)=>void;
}

/* =========================================================== */
const DynamicStatus: FC<DynamicStatusProps> = ({
  formName, data, wsData, connected,
  originId, updateWsData
}) => {

  /* ---------- state ---------- */
  const [formState,setFormState] = useState<Form>({
    title:formName, description:'', fields:[]
  });

  const editingFields = useRef(new Set<string>());

  /* ========= helper ========= */
  const getVal = (src:any, key:string) =>
        src?.[key] ?? src?.status?.fields?.[key];

  const mergeTrend = (
    oldArr:any[],
    fresh:any[],
    maxPts:number
  )=>{
    const merged = [...oldArr];
    fresh.forEach(pt=>{
      const ts = pt.timestamp;
      const idx = merged.findIndex((it:any)=>it.timestamp===ts);
      if(idx===-1){
        merged.push({...pt});
      }else{
        merged[idx] = {...merged[idx], ...pt};
      }
    });
    merged.sort((a:any,b:any)=>a.timestamp-b.timestamp);
    if(maxPts && merged.length>maxPts)
      merged.splice(0, merged.length-maxPts);
    return merged;
  };

  /* ========= REST → initial ========= */
  useEffect(()=>{
    if(!data?.fields) return;

    const fields : Field[] =
      data.fields.map((obj,idx)=>{
        const o = obj.o ?? '';
        const base = Object.keys(obj)[0];
        let val    = obj[base];

        const {optionMap, finalType} = parseFieldOptions(base,o);
        const label = finalType==='trend'
          ? `${base}_${(o.match(/mode=([^;]+)/)?.[1]||idx)}`
          : base;

        if(finalType==='checkbox'||finalType==='switch')
          val = (val===1||val==='1');

        return {
          label,
          value: val,
          type : finalType,
          readOnly   : !!optionMap.r,
          min        : optionMap.mn ? parseFloat(optionMap.mn.value as string) : undefined,
          max        : optionMap.mx ? parseFloat(optionMap.mx.value as string) : undefined,
          step       : optionMap.st !== undefined ? Number(optionMap.st.value) : undefined,
          placeholder: optionMap.pl?.value as string|undefined,
          o,
          baseLabel  : base           // для WS-пошуку
        } as Field & {baseLabel:string};
      });

    setFormState({title:formName, description:data.description, fields});
  },[data,formName]);

  /* ========= WS apply ========= */
  const applyWs = useCallback((ws:any)=>{
    setFormState(prev=>{
      const upd = prev.fields.map(f=>{
        const {label,type,o} = f;
        const base = (f as any).baseLabel || label;

        if(type==='trend'){
          const inc = getVal(ws, base);
          if(!Array.isArray(inc)||!inc.length) return f;

          const oldArr = Array.isArray(f.value)? f.value : [];
          const maxPts = +(o.match(/maxPoints=(\d+)/)?.[1]||0);

          return {...f, value: mergeTrend(oldArr, inc, maxPts)};
        }

        const v = getVal(ws, base);
        if(v===undefined) return f;

        if(type==='checkbox'||type==='switch')
          return {...f, value:(v===1||v==='1')};

        return {...f, value:v};
      });
      return {...prev, fields:upd};
    });
  },[]);

  useEffect(()=>{ if(wsData) applyWs(wsData); },[wsData,applyWs]);

  /* ========= input handlers ========= */
  const onChange = (lbl:string,val:any)=>{
    editingFields.current.add(lbl);
    setFormState(p=>({...p,fields:p.fields.map(f=>f.label===lbl?{...f,value:val}:f)}));
  };

  const onBlur = (lbl:string,val:any)=>{
    const fld = formState.fields.find(f=>f.label===lbl);
    if(!fld) return;
    const v = (fld.type==='checkbox'||fld.type==='switch') ? (val?1:0) : val;
    updateWsData({type:'p', origin_id:originId, p:{[lbl]:v}}, true);
    setLiveValue(lbl, v, 'ws');
  };

  /* ========= render ========= */
  if(!formState.fields.length)
    return <FormLoader message="Loading..." />;

  return (
    <SectionContent title={formState.title||'Status'} titleGutter>
      {connected
        ? <DynamicContentHandler
            title={formState.title}
            description={formState.description}
            fields={formState.fields}
            onInputChange={onChange}
            onFieldBlur={onBlur}
            hideTitle
          />
        : <FormLoader message="Waiting for WebSocket connection..." />}
    </SectionContent>
  );
};

export default DynamicStatus;
