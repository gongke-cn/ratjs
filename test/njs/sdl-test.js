import {
    SDL_Init, SDL_Quit, SDL_INIT_EVERYTHING, SDL_WM_SetCaption, 
    SDL_GetVideoInfo, SDL_ListModes, SDL_SetVideoMode, SDL_GetVideoSurface, SDL_WM_ToggleFullScreen,
    SDL_Event, SDL_PollEvent, SDL_QUIT, SDL_KEYDOWN, SDL_MOUSEBUTTONDOWN, SDL_MOUSEBUTTONUP,
    SDL_FillRect, SDL_Rect, SDL_MapRGB, SDL_Flip, SDL_BlitSurface, SDL_AllocSurface, SDL_FreeSurface,
    SDLK_t, SDLK_q, SDLK_c
} from "SDL";

function main (program, width = 640, height = 480, bpp = 32)
{
    SDL_Init(SDL_INIT_EVERYTHING);

    let vi = SDL_GetVideoInfo();

    print(`video information: ${JSON.stringify(SDL_GetVideoInfo(), null, "  ")}\n`);

    let modes = SDL_ListModes(vi.vfmt, 0);

    if (modes == -1) {
        print("any dimension is okay\n");
    } else {
        print(`modes: ${modes}\n`);
    }

    if (SDL_SetVideoMode(width, height, bpp, 0) == null) {
        prerr(`set mode ${width} x ${height} ${bpp}bpp failed\n`);
        return 1;
    }

    SDL_WM_SetCaption("sdl-test");

    let s = SDL_GetVideoSurface();
    let e = new SDL_Event();
    let startX, startY;

event_loop: while (true) {
        if (SDL_PollEvent(e)) {
            switch (e.type) {
            case SDL_QUIT:
                break event_loop;
            case SDL_KEYDOWN:
                switch (e.key.keysym.sym) {
                case SDLK_t:
                    SDL_WM_ToggleFullScreen(s);
                    break;
                case SDLK_q:
                    break event_loop;
                case SDLK_c:
                    let b = SDL_AllocSurface(0, s.w, s.h, s.format.BitsPerPixel, s.format.Rmask, s.format.Gmask, s.format.Bmask, s.format.Amask);
                    let r = new SDL_Rect({x:0, y:0, w:width, h:height});
                    SDL_BlitSurface(s, r, b, r);
                    for (let i = 0; i < height; i ++) {
                        SDL_BlitSurface(b, r, s, new SDL_Rect({x:i, y:i, w:width, h:height}));
                        SDL_Flip(s);
                    }
                    SDL_FreeSurface(b);
                    break;
                }
                break;
            case SDL_MOUSEBUTTONDOWN:
                ({x: startX, y: startY} = e.button);
                break;
            case SDL_MOUSEBUTTONUP:
                let {x: endX, y: endY} = e.button;
                let x1 = Math.min(startX, endX);
                let y1 = Math.min(startY, endY);
                let x2 = Math.max(startX, endX);
                let y2 = Math.max(startY, endY);

                let pix = SDL_MapRGB(s.format, Math.random()*255, Math.random()*255, Math.random()*255);

                SDL_FillRect(s, new SDL_Rect({x: x1, y: y1, w: x2 - x1, h: y2 - y1}), pix);
                SDL_Flip(s);
                break;
            }
        }
    }

    SDL_Quit();
}
