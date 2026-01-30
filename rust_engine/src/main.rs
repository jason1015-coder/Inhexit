use bevy::prelude::*;
use bevy::window::{PresentMode, WindowResolution};
use std::net::TcpStream;
use std::io::{BufRead, BufReader, Write};
use std::time::Duration;
use std::sync::{Arc, Mutex};
use std::f32::consts::PI;

const TCP_HOST: &str = "127.0.0.1";
const TCP_PORT: u16 = 9999;
const HEX_SIZE: f32 = 30.0;

// Game state structures
#[derive(Clone, Debug)]
struct GameState {
    in_game: bool,
    player: Option<PlayerState>,
    camera: Option<CameraState>,
    hexagons: Vec<HexagonState>,
    organisms: Vec<OrganismState>,
    particles: Vec<ParticleState>,
    inventory: std::collections::HashMap<String, u32>,
    game_mode: String,
}

#[derive(Clone, Debug)]
struct PlayerState {
    x: f32,
    y: f32,
    radius: f32,
    color: [u8; 3],
    selected_slot: i32,
    flight_mode: bool,
    on_ground: bool,
}

#[derive(Clone, Debug)]
struct CameraState {
    x: f32,
    y: f32,
}

#[derive(Clone, Debug)]
struct HexagonState {
    x: f32,
    y: f32,
    block_type: String,
    color: Vec<u8>,
    health: f32,
    max_health: f32,
    transparent: bool,
}

#[derive(Clone, Debug)]
struct OrganismState {
    x: f32,
    y: f32,
    org_type: String,
    health: f32,
    max_health: f32,
}

#[derive(Clone, Debug)]
struct ParticleState {
    x: f32,
    y: f32,
    color: Vec<u8>,
    age: f32,
    lifetime: f32,
    size: f32,
}

// TCP Client
struct TcpClient {
    stream: Arc<Mutex<TcpStream>>,
}

impl TcpClient {
    fn connect(host: &str, port: u16) -> Result<Self, std::io::Error> {
        let stream = TcpStream::connect((host, port))?;
        stream.set_read_timeout(Some(Duration::from_millis(100)))?;
        Ok(Self {
            stream: Arc::new(Mutex::new(stream)),
        })
    }

    fn send_message(&self, msg: &serde_json::Value) -> Result<(), std::io::Error> {
        let json_str = msg.to_string();
        let mut stream = self.stream.lock().unwrap();
        stream.write_all(json_str.as_bytes())?;
        stream.flush()?;
        Ok(())
    }

    fn receive_message(&self) -> Result<Option<serde_json::Value>, std::io::Error> {
        let mut stream = self.stream.lock().unwrap();
        let mut reader = BufReader::new(&mut *stream);
        let mut line = String::new();
        
        match reader.read_line(&mut line) {
            Ok(0) => Ok(None), // Connection closed
            Ok(_) => {
                let trimmed = line.trim();
                if trimmed.is_empty() {
                    Ok(None)
                } else {
                    serde_json::from_str(trimmed).map_err(|e| {
                        std::io::Error::new(std::io::ErrorKind::InvalidData, e)
                    }).map(Some)
                }
            }
            Err(ref e) if e.kind() == std::io::ErrorKind::WouldBlock => Ok(None),
            Err(e) => Err(e),
        }
    }
}

// Bevy Resources
#[derive(Resource)]
struct GameClient {
    client: Option<TcpClient>,
}

#[derive(Resource)]
struct InputState {
    keys: std::collections::HashMap<String, bool>,
    mining: bool,
    place_block: bool,
    mouse_position: (f32, f32),
}

#[derive(Resource)]
struct CurrentGameState {
    state: GameState,
}

// Bevy Components
#[derive(Component)]
struct Hexagon;

#[derive(Component)]
struct Organism;

#[derive(Component)]
struct Particle;

#[derive(Component)]
struct PlayerEntity;

fn main() {
    App::new()
        .add_plugins(DefaultPlugins.set(WindowPlugin {
            primary_window: Some(Window {
                title: "TesselBox - Bevy Engine".to_string(),
                resolution: WindowResolution::new(1280.0, 720.0),
                present_mode: PresentMode::AutoVsync,
                resizable: false,
                ..default()
            }),
            ..default()
        }))
        .insert_resource(ClearColor(Color::srgb(0.53, 0.81, 0.92)))
        .insert_resource(GameClient {
            client: TcpClient::connect(TCP_HOST, TCP_PORT).ok(),
        })
        .insert_resource(InputState {
            keys: std::collections::HashMap::new(),
            mining: false,
            place_block: false,
            mouse_position: (0.0, 0.0),
        })
        .insert_resource(CurrentGameState {
            state: GameState {
                in_game: false,
                player: None,
                camera: None,
                hexagons: Vec::new(),
                organisms: Vec::new(),
                particles: Vec::new(),
                inventory: std::collections::HashMap::new(),
                game_mode: "survival".to_string(),
            },
        })
        .add_systems(Startup, setup)
        .add_systems(Update, (
            handle_input,
            send_input_to_server,
            receive_state_from_server,
            update_entities,
        ))
        .add_systems(Update, update_camera)
        .run();
}

fn setup(mut commands: Commands) {
    // Camera
    commands.spawn(Camera2dBundle::default());
    
    // Player entity
    commands.spawn((
        PlayerEntity,
        SpriteBundle {
            sprite: Sprite {
                color: Color::srgb(0.2, 0.6, 1.0),
                custom_size: Some(Vec2::new(30.0, 30.0)),
                ..default()
            },
            transform: Transform::from_xyz(640.0, 360.0, 10.0),
            ..default()
        },
    ));
    
    println!("TesselBox Bevy Engine started!");
    println!("Make sure the Python bridge server is running on {}:{}", TCP_HOST, TCP_PORT);
}

fn handle_input(
    keys: Res<ButtonInput<KeyCode>>,
    mouse: Res<ButtonInput<MouseButton>>,
    mut input_state: ResMut<InputState>,
    windows: Query<&Window>,
) {
    // Key mappings
    input_state.keys.insert("w".to_string(), keys.pressed(KeyCode::KeyW));
    input_state.keys.insert("a".to_string(), keys.pressed(KeyCode::KeyA));
    input_state.keys.insert("s".to_string(), keys.pressed(KeyCode::KeyS));
    input_state.keys.insert("d".to_string(), keys.pressed(KeyCode::KeyD));
    input_state.keys.insert("space".to_string(), keys.pressed(KeyCode::Space));
    input_state.keys.insert("1".to_string(), keys.pressed(KeyCode::Digit1));
    input_state.keys.insert("2".to_string(), keys.pressed(KeyCode::Digit2));
    input_state.keys.insert("3".to_string(), keys.pressed(KeyCode::Digit3));
    input_state.keys.insert("4".to_string(), keys.pressed(KeyCode::Digit4));
    input_state.keys.insert("5".to_string(), keys.pressed(KeyCode::Digit5));
    input_state.keys.insert("6".to_string(), keys.pressed(KeyCode::Digit6));
    input_state.keys.insert("7".to_string(), keys.pressed(KeyCode::Digit7));
    input_state.keys.insert("8".to_string(), keys.pressed(KeyCode::Digit8));
    input_state.keys.insert("9".to_string(), keys.pressed(KeyCode::Digit9));
    input_state.keys.insert("equals".to_string(), keys.pressed(KeyCode::Equal));
    input_state.keys.insert("minus".to_string(), keys.pressed(KeyCode::Minus));
    input_state.keys.insert("f".to_string(), keys.pressed(KeyCode::KeyF));
    
    // Mouse
    input_state.mining = mouse.pressed(MouseButton::Left);
    input_state.place_block = mouse.pressed(MouseButton::Right);
    
    // Mouse position
    if let Ok(window) = windows.get_single() {
        if let Some(pos) = window.cursor_position() {
            input_state.mouse_position = (pos.x, pos.y);
        }
    }
}

fn send_input_to_server(
    game_client: Res<GameClient>,
    input_state: Res<InputState>,
) {
    if let Some(client) = &game_client.client {
        let message = serde_json::json!({
            "type": "update",
            "keys": &input_state.keys,
            "mining": input_state.mining,
            "place_block": input_state.place_block,
            "mouse_position": [input_state.mouse_position.0, input_state.mouse_position.1]
        });
        
        let _ = client.send_message(&message);
    }
}

fn receive_state_from_server(
    game_client: Res<GameClient>,
    mut current_state: ResMut<CurrentGameState>,
) {
    if let Some(client) = &game_client.client {
        if let Ok(Some(response)) = client.receive_message() {
            if response["type"] == "state" {
                current_state.state.in_game = response["in_game"].as_bool().unwrap_or(false);
                
                if current_state.state.in_game {
                    // Parse player
                    if let Some(player_data) = response.get("player") {
                        current_state.state.player = Some(PlayerState {
                            x: player_data["x"].as_f64().unwrap_or(0.0) as f32,
                            y: player_data["y"].as_f64().unwrap_or(0.0) as f32,
                            radius: player_data["radius"].as_f64().unwrap_or(15.0) as f32,
                            color: parse_color_array(player_data.get("color")),
                            selected_slot: player_data["selected_slot"].as_i64().unwrap_or(0) as i32,
                            flight_mode: player_data["flight_mode"].as_bool().unwrap_or(false),
                            on_ground: player_data["on_ground"].as_bool().unwrap_or(false),
                        });
                    }
                    
                    // Parse camera
                    if let Some(camera_data) = response.get("camera") {
                        current_state.state.camera = Some(CameraState {
                            x: camera_data["x"].as_f64().unwrap_or(0.0) as f32,
                            y: camera_data["y"].as_f64().unwrap_or(0.0) as f32,
                        });
                    }
                    
                    // Parse hexagons
                    current_state.state.hexagons.clear();
                    if let Some(hex_data) = response.get("hexagons") {
                        if let Some(hex_array) = hex_data.as_array() {
                            for hex in hex_array {
                                current_state.state.hexagons.push(HexagonState {
                                    x: hex["x"].as_f64().unwrap_or(0.0) as f32,
                                    y: hex["y"].as_f64().unwrap_or(0.0) as f32,
                                    block_type: hex["block_type"].as_str().unwrap_or("").to_string(),
                                    color: parse_color_vec(hex.get("color")),
                                    health: hex["health"].as_f64().unwrap_or(100.0) as f32,
                                    max_health: hex["max_health"].as_f64().unwrap_or(100.0) as f32,
                                    transparent: hex["transparent"].as_bool().unwrap_or(false),
                                });
                            }
                        }
                    }
                    
                    // Parse organisms
                    current_state.state.organisms.clear();
                    if let Some(org_data) = response.get("organisms") {
                        if let Some(org_array) = org_data.as_array() {
                            for org in org_array {
                                current_state.state.organisms.push(OrganismState {
                                    x: org["x"].as_f64().unwrap_or(0.0) as f32,
                                    y: org["y"].as_f64().unwrap_or(0.0) as f32,
                                    org_type: org["type"].as_str().unwrap_or("").to_string(),
                                    health: org["health"].as_f64().unwrap_or(100.0) as f32,
                                    max_health: org["max_health"].as_f64().unwrap_or(100.0) as f32,
                                });
                            }
                        }
                    }
                    
                    // Parse particles
                    current_state.state.particles.clear();
                    if let Some(part_data) = response.get("particles") {
                        if let Some(part_array) = part_data.as_array() {
                            for part in part_array {
                                current_state.state.particles.push(ParticleState {
                                    x: part["x"].as_f64().unwrap_or(0.0) as f32,
                                    y: part["y"].as_f64().unwrap_or(0.0) as f32,
                                    color: parse_color_vec(part.get("color")),
                                    age: part["age"].as_f64().unwrap_or(0.0) as f32,
                                    lifetime: part["lifetime"].as_f64().unwrap_or(1.0) as f32,
                                    size: part["size"].as_f64().unwrap_or(3.0) as f32,
                                });
                            }
                        }
                    }
                    
                    // Parse inventory
                    current_state.state.inventory.clear();
                    if let Some(inv_data) = response.get("inventory") {
                        if let Some(inv_obj) = inv_data.as_object() {
                            for (key, value) in inv_obj.iter() {
                                current_state.state.inventory.insert(
                                    key.clone(),
                                    value.as_u64().unwrap_or(0) as u32
                                );
                            }
                        }
                    }
                    
                    current_state.state.game_mode = response["game_mode"]
                        .as_str()
                        .unwrap_or("survival")
                        .to_string();
                }
            }
        }
    }
}

fn update_entities(
    mut commands: Commands,
    current_state: Res<CurrentGameState>,
    mut player_query: Query<&mut Transform, With<PlayerEntity>>,
    mut hex_query: Query<(Entity, &mut Transform), (With<Hexagon>, Without<PlayerEntity>)>,
) {
    // Update player
    if let Some(ref player) = current_state.state.player {
        if let Ok(mut transform) = player_query.get_single_mut() {
            transform.translation.x = player.x;
            transform.translation.y = player.y;
        }
    }
    
    // For simplicity, we're not creating/updating hexagon entities in this version
    // In a full implementation, we would manage entities more efficiently
}

fn update_camera(
    current_state: Res<CurrentGameState>,
    mut camera_query: Query<&mut Transform, (With<Camera2d>, Without<PlayerEntity>)>,
) {
    if let Some(ref camera) = current_state.state.camera {
        if let Ok(mut transform) = camera_query.get_single_mut() {
            // Convert camera from Python coordinate system to Bevy
            transform.translation.x = camera.x;
            transform.translation.y = camera.y;
        }
    }
}

fn parse_color_array(color_val: Option<&serde_json::Value>) -> [u8; 3] {
    if let Some(val) = color_val {
        if let Some(arr) = val.as_array() {
            if arr.len() >= 3 {
                return [
                    arr[0].as_u64().unwrap_or(255) as u8,
                    arr[1].as_u64().unwrap_or(255) as u8,
                    arr[2].as_u64().unwrap_or(255) as u8,
                ];
            }
        }
    }
    [255, 255, 255]
}

fn parse_color_vec(color_val: Option<&serde_json::Value>) -> Vec<u8> {
    if let Some(val) = color_val {
        if let Some(arr) = val.as_array() {
            return arr.iter().map(|v| v.as_u64().unwrap_or(255) as u8).collect();
        }
    }
    vec![255, 255, 255, 255]
}