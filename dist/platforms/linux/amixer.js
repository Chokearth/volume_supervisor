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
exports.linuxAmixer = void 0;
const commands_1 = require("../../utils/commands");
exports.linuxAmixer = {
    getGlobalVolume() {
        return __awaiter(this, void 0, void 0, function* () {
            const stdout = yield (0, commands_1.execCommand)('amixer', ['sget', 'Master']);
            if (!stdout)
                throw new Error('Failed to get volume');
            const channels = stdout.match(/(?<=\[)(\d+)(?=%\])/g); // This regex finds a digit(s) which is prefixed with "[" and suffixed with "%]"
            if (!channels.length)
                throw new Error('Failed to get volume');
            const sum = channels.reduce((acc, channel) => acc + parseInt(channel), 0);
            if (isNaN(sum))
                throw new Error('Failed to get volume');
            return sum / channels.length;
        });
    },
    setGlobalVolume(volume) {
        return __awaiter(this, void 0, void 0, function* () {
            if (volume < 0 || volume > 100)
                throw new Error('Volume must be between 0 and 100');
            yield (0, commands_1.execCommand)('amixer', ['set', 'Master', `${volume}%`]);
        });
    },
    isMuted: function () {
        return __awaiter(this, void 0, void 0, function* () {
            const stdout = yield (0, commands_1.execCommand)('amixer', ['sget', 'Master']);
            if (!stdout)
                throw new Error('Failed to get mute state');
            return stdout
                .match(/\[(on|off)\]/)
                .some((state) => state === 'off');
        });
    },
    setMuted(muted) {
        return __awaiter(this, void 0, void 0, function* () {
            yield (0, commands_1.execCommand)('amixer', ['set', 'Master', muted ? 'mute' : 'unmute']);
        });
    },
};
//# sourceMappingURL=amixer.js.map