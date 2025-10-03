// src/utils/useWs.ts

import { useCallback, useEffect, useRef, useState } from 'react';
import Sockette from 'sockette';
import { throttle } from 'lodash';
import { addAccessTokenParameter } from '../api/authentication';

/**
 * Типи для повідомлень WebSocket
 */
export interface WebSocketIdMessage {
  type: 'id';
  id: string;
  origin_id: string;
}

export interface WebSocketPayloadMessage<D> {
  type: 'p';
  origin_id: string;
  p: D;
}

export type WebSocketMessage<D> = WebSocketIdMessage | WebSocketPayloadMessage<D>;

/**
 * Хук useWs: створює та підтримує WebSocket-з'єднання
 * @param wsUrl - адреса WebSocket (з доданим токеном, якщо треба)
 * @param wsThrottle - затримка (ms) для throttle (уникання надмірної кількості повідомлень)
 * @param maxAttempts - максимальна кількість спроб перепідключення
 */
export const useWs = <D>(
  wsUrl: string,
  wsThrottle: number = 500,
  maxAttempts: number = 10
) => {
  const ws = useRef<Sockette>();
  const clientId = useRef<string>();

  // Стан з'єднання
  const [connected, setConnected] = useState<boolean>(false);
  // Поточний originId (отримуємо з повідомлень типу "id")
  const [originId, setOriginId] = useState<string>('');
  // Основні дані, прийняті через WebSocket (якщо "p" - payload)
  const [wsData, setWsData] = useState<D | undefined>();

  // Контроль передачі (чи зберегти дані на сервер), а також чи чистити локальні дані
  const [transmit, setTransmit] = useState<boolean>(false);
  const [clear, setClear] = useState<boolean>(false);

  /**
   * Обробник вхідних повідомлень WebSocket
   */
  const onMessage = useCallback((event: MessageEvent) => {
    const rawData = event.data;
    if (typeof rawData === 'string') {
      try {
        const message = JSON.parse(rawData) as WebSocketMessage<D>;
        console.log('[useWs] Received message:', message);

        switch (message.type) {
          case 'id':
            clientId.current = message.id;
            setOriginId(message.origin_id);
            break;
          case 'p':
            if (message.origin_id) {
              setOriginId(message.origin_id);
            }
            // Записуємо в локальний стан payload
            setWsData(message.p);
            break;
          default:
            console.warn(`[useWs] Unknown message type: ${message}`);
        }
      } catch (error) {
        console.error('[useWs] Error parsing message:', error);
      }
    }
  }, []);

  /**
   * Безпосередня відправка даних (приватний метод),
   * обгорнутий у throttle для обмеження частоти.
   */
  const doSaveData = useCallback(
    (newData: D, clearData: boolean = false) => {
      if (!ws.current) return;
      if (clearData) {
        setWsData(undefined);
      }
      ws.current.json(newData);
    },
    []
  );

  // Створюємо throttled-функцію відправки
  const saveData = useRef(throttle(doSaveData, wsThrottle));

  /**
   * Публічний метод оновлення локального стану та ініціації відправки
   * @param newData - нові дані (або callback)
   * @param transmitData - чи відправляти відразу на сервер
   * @param clearData - чи стирати локальний wsData
   */
  const updateData = (
    newData: React.SetStateAction<D | undefined>,
    transmitData: boolean = true,
    clearData: boolean = false
  ) => {
    setWsData(newData);
    setTransmit(transmitData);
    setClear(clearData);
  };

  /**
   * Відключення WebSocket
   */
  const disconnect = useCallback(() => {
    if (ws.current) {
      ws.current.close();
    }
  }, []);

  /**
   * Ефект відправки даних: якщо transmit = true, викликаємо saveData
   */
  useEffect(() => {
    if (!transmit) return;
    if (wsData) {
      saveData.current(wsData, clear);
    }
    setTransmit(false);
    setClear(false);
  }, [wsData, transmit, clear]);

  /**
   * Підключення до WebSocket (та повторні спроби)
   */
  useEffect(() => {
    let attempts = 0;

    const instance = new Sockette(addAccessTokenParameter(wsUrl), {
      onmessage: onMessage,
      onopen: () => {
        setConnected(true);
        attempts = 0;
        console.log('[useWs] WebSocket connected');
      },
      onclose: () => {
        clientId.current = undefined;
        setConnected(false);
        setWsData(undefined); // очищуємо локальний wsData
        console.warn('[useWs] WebSocket disconnected');
        attempts++;
        if (attempts > maxAttempts) {
          console.warn('useWs - Stop attempts! Reached max attempts:', attempts);
        } else {
          console.log(`useWs - Attempt ${attempts} to reconnect...`);
        }
      },
      onerror: (error) => {
        console.error('useWs - WebSocket error:', error);
      },
      timeout: 5000,
      maxAttempts: maxAttempts,
    });

    ws.current = instance;

    // При демонтовані компонента закриваємо WS
    return () => {
      instance.close();
    };
  }, [wsUrl, onMessage, maxAttempts]);

  return {
    connected,
    originId,
    wsData,
    updateData,
    disconnect,
  } as const;
};
