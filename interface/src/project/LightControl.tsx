import { FC, useEffect, useState } from 'react';
import { Navigate, Route, Routes } from 'react-router-dom';
import { Tab } from '@mui/material';
import { RouterTabs, useRouterTab, useLayoutTitle } from '../components';
import DynamicStatus from '../components/dynamic-component/DynamicStatus';
import DynamicSettings from '../components/dynamic-component/DynamicSettings';
import { readState, updateState, LIGHT_SETTINGS_WEBSOCKET_URL } from './api';
import { useRest } from '../utils';
import { useWs } from '../utils/useWs';
import FormLoader from '../components/loading/FormLoader';

const LightControl: FC = () => {
  useLayoutTitle("Light Control");
  const { routerTab } = useRouterTab();
  const { loadData, saveData, saving, setData, data, errorMessage } = useRest<any>({
    read: readState,
    update: updateState,
  });

  // Отримуємо wsData через useWs
  const { connected, wsData, updateData: updateWsData, disconnect } = useWs<any>(LIGHT_SETTINGS_WEBSOCKET_URL);

  // Зберігаємо origin_id у локальному стані
  const [originId, setOriginId] = useState<string>("");
  useEffect(() => {
    if (wsData && wsData.origin_id) {
      setOriginId(wsData.origin_id);
    }
  }, [wsData]);

  useEffect(() => {
    loadData();
  }, [loadData]);

  const handleUpdateData = (formName: string, newData: Partial<any>) => {
    setData((prevData: any) => ({
      ...prevData,
      [formName]: {
        ...prevData[formName],
        ...newData
      }
    }));
  };

  const handleSaveData = (updatedData: Partial<any>) => {
    updateState(updatedData)
      .then(() => {
        saveData();
      })
      .catch((err) => {
        console.error(`Error saving data:`, err);
      });
  };

  if (!data) {
    return <FormLoader errorMessage={errorMessage} message="Loading REST data..." />;
  }

  return (
    <>
      <RouterTabs value={routerTab}>
        <Tab value="status" label="Status" />
        <Tab value="settings" label="Settings" />
      </RouterTabs>
      <Routes>
        <Route 
          path="status" 
          element={
            <DynamicStatus
              formName="Status"
              data={data.status}
              wsData={wsData}
              connected={connected}
              saving={saving}
              originId={originId}  // Передаємо origin_id
              setData={(newData: any) => handleUpdateData('status', newData)}
              disconnect={disconnect}
              updateWsData={updateWsData}
            />
          }
        />
        <Route 
          path="settings" 
          element={
            <DynamicSettings 
              formName="Settings"
              data={data.settings} 
              saveData={(updatedData: any) => handleSaveData(updatedData)} 
              saving={saving} 
              setData={(newData: any) => handleUpdateData('settings', newData)}
            />
          } 
        />
        <Route path="/*" element={<Navigate replace to="status" />} />
      </Routes>
    </>
  );
};

export default LightControl;
