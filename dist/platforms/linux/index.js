"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.linux = void 0;
const child_process = require("child_process");
const amixer_1 = require("../../platforms/linux/amixer");
const pactl_1 = require("../../platforms/linux/pactl");
let pactlCompatible;
try {
    const stdout = child_process.execSync('command -v pactl 2>/dev/null' +
        ' && { echo >&1 pactl; exit 0; }').toString();
    pactlCompatible = !!stdout;
}
catch (e) {
    pactlCompatible = false;
}
exports.linux = pactlCompatible ? pactl_1.linuxPactl : amixer_1.linuxAmixer;
//# sourceMappingURL=index.js.map