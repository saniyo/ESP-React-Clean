// interface/src/project/ProjectRouting.tsx
import { FC } from 'react';
import { Navigate, Routes, Route } from 'react-router-dom';
import DemoProject from './DemoProject';
import LightControl from './LightControl';

const ProjectRouting: FC = () => {
  return (
    <Routes>
      <Route path="/*" element={<Navigate to="demo/information" />} />
      <Route path="demo/*" element={<DemoProject />} />
      <Route path="light-control/*" element={<LightControl />} />
    </Routes>
  );
};

export default ProjectRouting;
