import { AxiosPromise } from "axios";

import { AXIOS, WEB_SOCKET_ROOT } from "../../api/endpoints";


export const WEBSOCKET_URL = WEB_SOCKET_ROOT + "telegramStatus";

export function readState(): AxiosPromise<Record<string, any>> {
  return AXIOS.get('/telegramForm');
}

export function updateState(data: Record<string, any>): AxiosPromise<Record<string, any>> {
  return AXIOS.post('/telegramForm', data);
}
