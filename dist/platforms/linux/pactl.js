"use strict";
var __awaiter = (this && this.__awaiter) || function (thisArg, _arguments, P, generator) {
    function adopt(value) { return value instanceof P ? value : new P(function (resolve) { resolve(value); }); }
    return new (P || (P = Promise))(function (resolve, reject) {
        function fulfilled(value) { try { step(generator.next(value)); } catch (e) { reject(e); } }
        function rejected(value) { try { step(generator["throw"](value)); } catch (e) { reject(e); } }
        function step(result) { result.done ? resolve(result.value) : adopt(result.value).then(fulfilled, rejected); }
        step((generator = generator.apply(thisArg, _arguments || [])).next());
    });
};
Object.defineProperty(exports, "__esModule", { value: true });
exports.linuxPactl = void 0;
exports.linuxPactl = {
    getGlobalVolume() {
        return __awaiter(this, void 0, void 0, function* () {
            throw new Error('Not implemented');
            return 0;
        });
    },
    setGlobalVolume(volume) {
        return __awaiter(this, void 0, void 0, function* () {
            throw new Error('Not implemented');
        });
    },
    isMuted: function () {
        return __awaiter(this, void 0, void 0, function* () {
            throw new Error('Not implemented');
            return false;
        });
    },
    setMuted(muted) {
        return __awaiter(this, void 0, void 0, function* () {
            throw new Error('Not implemented');
        });
    },
};
//# sourceMappingURL=pactl.js.map