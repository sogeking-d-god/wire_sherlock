import { RouterProvider } from 'react-router';
import { router } from './routes';
import { FileProvider } from './context/FileContext';
import { Toaster } from 'sonner';

export default function App() {
  return (
    <FileProvider>
      <RouterProvider router={router} />
      <Toaster
        position="top-right"
        theme="dark"
        toastOptions={{
          style: {
            background: '#060e20',
            border: '1px solid #31394d',
            color: '#e2e8f0',
          },
        }}
      />
    </FileProvider>
  );
}