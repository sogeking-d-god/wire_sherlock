import { createBrowserRouter } from 'react-router';
import { RootLayout } from './components/RootLayout';
import { FileManagement } from './components/FileManagement';
import { AnalysisView } from './components/AnalysisView';

export const router = createBrowserRouter([
  {
    path: '/',
    Component: RootLayout,
    children: [
      {
        index: true,
        Component: FileManagement,
      },
      {
        path: 'files',
        Component: FileManagement,
      },
      {
        path: 'dashboard',
        Component: AnalysisView,
      },
    ],
  },
]);
