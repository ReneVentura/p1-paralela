#include <cstdlib>
#include <ctime>
#include <iostream>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <cmath>
#include <chrono>

const int windowHeight = 480;
const int windowWidth = 640;
const int starRadius = 30;
const float gravity = 150.0f;
const float collision = 0.5f;
const float elasticity = 0.8f;


//calcular tiempo
Uint32 start_time_operations, end_time_operations;
Uint64 total_time_operations = 0;


Uint32 start_time_collision = SDL_GetTicks(); //timer para las colisiones de cada bola
struct Star {
    int x, y, dx, dy, bounces, radius;
    Uint8 r, g, b; // colores
    Uint32 start_time_collision;

};

//ver si las dos bolas colisionan usando las coordenadas x y de cada una para sacar la distancia entre sus centros, ignorecollision se usa para que cuando sea true, no pase por el check.
bool isCollision(Star &star1, Star &star2, bool ignoreCollision) {
    if (ignoreCollision) {
        return false;
    }
    int dx = star1.x - star2.x;
    int dy = star1.y - star2.y;
    int distanceSquared = dx * dx + dy * dy;
    return distanceSquared <= (star1.radius + star2.radius) * (star1.radius + star2.radius);
}

//DSe dibujan estrellas usando math.h y sdl.
void drawStar(SDL_Renderer* renderer, int x, int y, int radius) {
    const int num_points = 5;
    const double angle = 2 * M_PI / num_points;
    SDL_Point points[num_points * 2 + 1];

    for (int i = 0; i < num_points * 2; i += 2) {
        points[i].x = x + radius * cos(i / 2 * angle);
        points[i].y = y + radius * sin(i / 2 * angle);
        points[i + 1].x = x + (radius / 2) * cos((i / 2) * angle + angle / 2);
        points[i + 1].y = y + (radius / 2) * sin((i / 2) * angle + angle / 2);
    }

    points[num_points * 2] = points[0];
    SDL_RenderDrawLines(renderer, points, num_points * 2 + 1);
}
//cada id es un objeto en la colision y el timestamp es el tiempo de la misma
struct Collision {
    int id1, id2;
    Uint32 timestamp;
};


/*
Esta función gestiona la colisión entre dos bolas. Toma dos objetos Ball como parámetros de entrada y actualiza sus posiciones y velocidades en función de la física de la colisión.

Primero, calcula la distancia entre las dos bolas usando el teorema de Pitágoras y verifica los casos extremos donde la distancia es cero o NaN. Luego, calcula la superposición entre las bolas y el vector normal (nx, ny) de la colisión. Usando estos valores, ajusta las posiciones de las bolas para evitar que se superpongan.

A continuación, calcula la velocidad relativa de las dos bolas a lo largo del vector normal y calcula el impulso, que es el cambio de cantidad de movimiento causado por la colisión. Usando el impulso, actualiza las velocidades de las dos bolas.

Finalmente, agrega una función para reducir el tamaño de las bolas cuando chocan y cambia su color aleatoriamente. También actualiza las marcas de tiempo de colisión de las dos bolas para rastrear cuándo ocurrió la colisión.
*/
void starCollisionManager(Star &star1, Star &star2) {
    int dx = star1.x - star2.x;
    int dy = star1.y - star2.y;
    float distance = hypot(dx, dy);

    if (distance == 0.0f) {
        return;
    }
    if (std::isnan(distance)) {
        std::cerr << "Error: invalid distance value: dx = " << dx << ", dy = " << dy << std::endl;
        return;
    }

    float overlap = star1.radius + star2.radius - distance;
    float nx = dx / distance;
    float ny = dy / distance;

    star1.x += nx * overlap / 2;
    star1.y += ny * overlap / 2;
    star2.x -= nx * overlap / 2;
    star2.y -= ny * overlap / 2;

    float relativeVelocityX = star1.dx - star2.dx;
    float relativeVelocityY = star1.dy - star2.dy;
    float impulse = (relativeVelocityX * nx + relativeVelocityY * ny) * elasticity;

    star1.dx -= nx * impulse;
    star1.dy -= ny * impulse;
    star2.dx += nx * impulse;
    star2.dy += ny * impulse;


    if (star1.radius > starRadius/2 && star2.radius > starRadius/2) {
        star1.radius *= collision;
        star2.radius *= collision;

        star1.r = rand() % 256;
        star1.g = rand() % 256;
        star1.b = rand() % 256;
        star2.r = rand() % 256;
        star2.g = rand() % 256;
        star2.b = rand() % 256;
    }
    star1.start_time_collision = SDL_GetTicks();
    star2.start_time_collision = SDL_GetTicks();
}

//Inicia el array de las estrellas, agrega el contador de frames igual que su avg, inicia al window y el renderer. Pide cuantas bolas se quieren
int main(int argc, char* argv[]) {
    // Initialize TTF
    if (TTF_Init() == -1) {
        std::cerr << "Error initializing TTF: " << TTF_GetError() << std::endl;
        return 1;
    }

    // Create font object
    TTF_Font* font = TTF_OpenFont("arial.ttf", 24);
    if (!font) {
        std::cerr << "Error loading font: " << TTF_GetError() << std::endl;
        return 1;
    }

   
    SDL_Init(SDL_INIT_VIDEO);

    // Create a window and renderer
    SDL_Window* window = SDL_CreateWindow("Screensaver", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, windowWidth, windowHeight, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    // Agregar contador de frames totales y tiempo de inicio
    int total_frames = 0;
    Uint32 program_start_time = SDL_GetTicks();
    
   
    Uint32 start_time = SDL_GetTicks();
    int frame_count = 0;
    int fps = 0;


    int num_stars;
    std::string input;
    bool valid_input = false;
    while (!valid_input) {
        std::cout << "Ingrese el numero de estrellas tiene que ser mayor a 0: ";
        std::cin >> num_stars;
        if (std::cin.fail() || num_stars <= 0) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "el numero de estrellas tiene que ser mayor a 0." << std::endl;
        } else {
            valid_input = true;
        }
    }

    // Set up an array to hold the positions and velocities of each ball

    Star* stars = nullptr;
    stars = new Star[num_stars];
    // Initialize the positions and velocities of each ball
    srand(time(0)); // Seed the random number generator with 0
    for (int i = 0; i < num_stars; i++) {
        stars[i].x = rand() % (windowWidth - starRadius * 2) + starRadius;
        stars[i].y = starRadius;
        stars[i].dx = rand() % 400 - 200;
        stars[i].dy = rand() % 400 - 200;
        stars[i].bounces = 0;
        stars[i].radius = starRadius;
        stars[i].r = rand() % 256;
        stars[i].g = rand() % 256;
        stars[i].b = rand() % 256;
        stars[i].start_time_collision = SDL_GetTicks();

    }

/*
ciclo principal que maneja eventos y actualiza la posición y velocidad de las bolas. Primero comprueba si hay eventos en la cola de eventos de SDL y establece 
el indicador de salida en verdadero si el usuario cierra la ventana. Luego actualiza la posición de cada bola sumando su velocidad a su posición y aplica 
gravedad a la velocidad de cada bola. Después de eso, comprueba si hay colisiones con las paredes de la ventana y con otras bolas, 
y actualiza sus velocidades en consecuencia. Si una pelota rebota en el suelo, pierde algo de velocidad debido a la fricción. 
Finalmente, comprueba si una bola ha rebotado demasiadas veces o ha alcanzado un radio mínimo, y la elimina de la matriz de bolas, 
creando una nueva en su lugar con parámetros aleatorios iniciales.
*/
bool quit = false;
while (!quit) {
    // Handle events
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            quit = true;
        }
    }
    
    Uint32 elapsed_time_collision = SDL_GetTicks() - start_time_collision;
    bool ignoreCollision = elapsed_time_collision < 5000;

    // Clear the screen
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    //dibuja las estrellas
    //registramos el tiempo de inicio de las operaciones
    auto start_timep = std::chrono::high_resolution_clock::now();
    start_time_operations = SDL_GetTicks();
    for (int i = 0; i < num_stars; i++) {
        // Update position
        stars[i].x += stars[i].dx / 60;
        stars[i].y += stars[i].dy / 60;
        

        // Apply grav
        stars[i].dy += gravity / 60;

        // Handle wall collisions
        if (stars[i].x - stars[i].radius < 0) {
            stars[i].x = stars[i].radius;
            stars[i].dx = -stars[i].dx * collision;
        } else if (stars[i].x + stars[i].radius > windowWidth) {
            stars[i].x = windowWidth - stars[i].radius;
            stars[i].dx = -stars[i].dx * collision;
        }
        // Handle ball collisions
        for (int j = i + 1; j < num_stars; j++) {
            Uint32 elapsed_time_collision_i = SDL_GetTicks() - stars[i].start_time_collision;
            Uint32 elapsed_time_collision_j = SDL_GetTicks() - stars[j].start_time_collision;
            bool ignoreCollision_i = elapsed_time_collision_i < 5000;
            bool ignoreCollision_j = elapsed_time_collision_j < 5000;
            bool ignoreCollision = ignoreCollision_i || ignoreCollision_j;
            
            if (isCollision(stars[i], stars[j], ignoreCollision)) {
                starCollisionManager(stars[i], stars[j]);
            }
        }
        if (stars[i].y - stars[i].radius < 0) {
            stars[i].y = stars[i].radius;
            stars[i].dy = -stars[i].dy * collision;

        } else if (stars[i].y + stars[i].radius > windowHeight) {
            stars[i].y = windowHeight - stars[i].radius;
            stars[i].dy = -stars[i].dy * collision;
            stars[i].bounces++;
            stars[i].radius -= 5;
            stars[i].r -= 25;
            stars[i].g -= 10;
            stars[i].b -= 5;
            stars[i].dx *= 1.2;
            stars[i].dy *= 1.2;
        }
       
        if (stars[i].x < stars[i].radius || stars[i].x > windowWidth - stars[i].radius) {
            stars[i].dx = -stars[i].dx * collision;
            if (fps != 0){


                stars[i].x += stars[i].dx / fps;
            }
            stars[i].bounces++;
        }
        if (stars[i].y < stars[i].radius) {
            stars[i].dy = -stars[i].dy * collision;
            if (fps != 0){
                stars[i].y += stars[i].dy / fps;
            }
            stars[i].bounces++;
        }
        if (stars[i].y > windowHeight - stars[i].radius) {
            stars[i].dy = -stars[i].dy * collision;
            if (fps != 0){
                stars[i].y += stars[i].dy / fps;
            }
            stars[i].bounces++;
        }
        float friction = 0.98f;
        if (stars[i].y + stars[i].radius >= windowHeight) {
            stars[i].dx *= friction;
        }

     // si la estrella rebota 3 o mas veces
        if (stars[i].bounces >= 3) {
            // reinicia las configuraciones de la estrella
            stars[i].x = rand() % (windowWidth / 2) + windowWidth / 4;
            stars[i].y = rand() % (windowHeight / 2) + windowHeight / 4;
            stars[i].dx = rand() % 400 - 200;
            stars[i].dy = rand() % 400 - 200;
            stars[i].bounces = 0;
        }
        if (stars[i].bounces > 10 || stars[i].radius <= 0) {
            // Remove the ball from the array by shifting all subsequent elements back by one
            for (int j = i; j < num_stars - 1; j++) {
                stars[j] = stars[j + 1];
            }
            num_stars--;
             // crea nuevas estrellas
            Star new_ball;
            new_ball.x = rand() % (windowWidth - starRadius * 2) + starRadius;
            new_ball.y = starRadius;
            new_ball.dx = rand() % 400 - 200;
            new_ball.dy = rand() % 400 - 200;
            new_ball.bounces = 0;
            new_ball.radius = starRadius;
            new_ball.r = rand() % 256;
            new_ball.g = rand() % 256;
            new_ball.b = rand() % 256;
            stars[num_stars] = new_ball;
            num_stars++;
        }
        

        // renderiza cada estrella
        SDL_SetRenderDrawColor(renderer, stars[i].r, stars[i].g, stars[i].b, 255);
       
        drawStar(renderer, stars[i].x, stars[i].y, stars[i].radius);

        for (int j = i + 1; j < num_stars; j++) {
            if (isCollision(stars[i], stars[j], ignoreCollision)) {
                starCollisionManager(stars[i], stars[j]);
            }
        }
        
    }
    auto end_time = std::chrono::high_resolution_clock::now();
    auto total_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_timep).count();
    std::cout << "Tiempo total de ejecucion " << total_time << " ms" << std::endl;
    end_time_operations = SDL_GetTicks();
    total_time_operations += (end_time_operations - start_time_operations);


    // Draw the FPS counter
    frame_count++;
    Uint32 elapsed_time = SDL_GetTicks() - start_time;    
    if (elapsed_time >= 1000) {
        fps = frame_count;
        frame_count = 0;
        start_time = SDL_GetTicks();
    }

    SDL_Color color = {255, 255, 255};
    std::string fps_text = "FPS: " + std::to_string(fps);
    SDL_Surface* surface = TTF_RenderText_Solid(font, fps_text.c_str(), color);
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_Rect text_rect = {0, 0, surface->w, surface->h};
    SDL_RenderCopy(renderer, texture, NULL, &text_rect);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);

    // Update the screen
    SDL_RenderPresent(renderer);
    total_frames++;  // Incrementar contador de frames totales
    }
    Uint32 program_elapsed_time = SDL_GetTicks() - program_start_time;
    float avg_fps = static_cast<float>(total_frames) / (program_elapsed_time / 1000.0f);

  
    std::cout << "Promedio de FPS: " << avg_fps << std::endl;

    


    delete[] stars; // Libera la memoria dinámica asignada al array de bolas
    // Clean up
    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;

}