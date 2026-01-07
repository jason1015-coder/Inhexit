"""
Hexagonal World Multiplayer Server
WebSocket-based server for seamless multiplayer integration
"""

import asyncio
import websockets
import json
import logging
from typing import Dict, Set, Optional
from dataclasses import dataclass, asdict
from datetime import datetime

# Configure logging
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

@dataclass
class PlayerState:
    """Represents a player's state in the world"""
    id: str
    name: str
    x: float
    y: float
    velocity_x: float
    velocity_y: float
    selected_block: int
    color: tuple
    
@dataclass
class BlockUpdate:
    """Represents a block change in the world"""
    q: int
    r: int
    block_type: int
    player_id: str

@dataclass
class ItemDrop:
    """Represents a dropped item in the world"""
    id: str
    q: int
    r: int
    block_type: int
    quantity: int
    velocity_x: float
    velocity_y: float

class GameServer:
    """Main multiplayer server class"""
    
    def __init__(self, host: str = "0.0.0.0", port: int = 8765):
        self.host = host
        self.port = port
        self.players: Dict[str, PlayerState] = {}
        self.clients: Dict[str, websockets.WebSocketServerProtocol] = {}
        self.block_updates: list = []
        self.dropped_items: Dict[str, ItemDrop] = {}
        self.running = False
        
        # World state (simplified for multiplayer)
        self.world_width = 1000
        self.world_height = 120
        
    async def handle_client(self, websocket: websockets.WebSocketServerProtocol, path: str):
        """Handle a new client connection"""
        client_id = id(websocket)
        self.clients[client_id] = websocket
        
        logger.info(f"New client connected: {client_id}")
        
        try:
            # Send initial world state
            await self.send_initial_state(websocket)
            
            # Main message loop
            async for message in websocket:
                await self.handle_message(client_id, message)
                
        except websockets.exceptions.ConnectionClosed:
            logger.info(f"Client disconnected: {client_id}")
        except Exception as e:
            logger.error(f"Error handling client {client_id}: {e}")
        finally:
            # Cleanup
            await self.remove_player(client_id)
            if client_id in self.clients:
                del self.clients[client_id]
    
    async def send_initial_state(self, websocket: websockets.WebSocketServerProtocol):
        """Send initial world state to new client"""
        initial_state = {
            "type": "initial_state",
            "world": {
                "width": self.world_width,
                "height": self.world_height
            },
            "players": [asdict(p) for p in self.players.values()],
            "block_updates": self.block_updates[-100:],  # Last 100 updates
            "dropped_items": [asdict(item) for item in self.dropped_items.values()]
        }
        
        await websocket.send(json.dumps(initial_state))
    
    async def handle_message(self, client_id: str, message: str):
        """Handle incoming message from client"""
        try:
            data = json.loads(message)
            msg_type = data.get("type")
            
            if msg_type == "player_join":
                await self.handle_player_join(client_id, data)
            elif msg_type == "player_update":
                await self.handle_player_update(client_id, data)
            elif msg_type == "block_place":
                await self.handle_block_place(client_id, data)
            elif msg_type == "block_break":
                await self.handle_block_break(client_id, data)
            elif msg_type == "item_drop":
                await self.handle_item_drop(client_id, data)
            elif msg_type == "item_pickup":
                await self.handle_item_pickup(client_id, data)
            else:
                logger.warning(f"Unknown message type: {msg_type}")
                
        except json.JSONDecodeError:
            logger.error(f"Invalid JSON from client {client_id}")
        except Exception as e:
            logger.error(f"Error handling message from client {client_id}: {e}")
    
    async def handle_player_join(self, client_id: str, data: dict):
        """Handle new player joining"""
        player_id = data.get("player_id", client_id)
        player_name = data.get("player_name", "Player")
        x = data.get("x", 0.0)
        y = data.get("y", 0.0)
        color = tuple(data.get("color", (255, 100, 100)))
        
        player = PlayerState(
            id=player_id,
            name=player_name,
            x=x,
            y=y,
            velocity_x=0.0,
            velocity_y=0.0,
            selected_block=1,
            color=color
        )
        
        self.players[player_id] = player
        
        # Broadcast to all clients
        await self.broadcast_message({
            "type": "player_joined",
            "player": asdict(player)
        })
        
        logger.info(f"Player joined: {player_name} ({player_id})")
    
    async def handle_player_update(self, client_id: str, data: dict):
        """Handle player state update"""
        player_id = data.get("player_id")
        
        if player_id in self.players:
            player = self.players[player_id]
            player.x = data.get("x", player.x)
            player.y = data.get("y", player.y)
            player.velocity_x = data.get("velocity_x", player.velocity_x)
            player.velocity_y = data.get("velocity_y", player.velocity_y)
            player.selected_block = data.get("selected_block", player.selected_block)
            
            # Broadcast to other clients
            await self.broadcast_to_others(client_id, {
                "type": "player_update",
                "player_id": player_id,
                "x": player.x,
                "y": player.y,
                "velocity_x": player.velocity_x,
                "velocity_y": player.velocity_y,
                "selected_block": player.selected_block
            })
    
    async def handle_block_place(self, client_id: str, data: dict):
        """Handle block placement"""
        q = data.get("q")
        r = data.get("r")
        block_type = data.get("block_type")
        player_id = data.get("player_id")
        
        update = BlockUpdate(q=q, r=r, block_type=block_type, player_id=player_id)
        self.block_updates.append(asdict(update))
        
        # Keep only last 1000 updates
        if len(self.block_updates) > 1000:
            self.block_updates = self.block_updates[-1000:]
        
        # Broadcast to all clients
        await self.broadcast_message({
            "type": "block_placed",
            "update": asdict(update)
        })
    
    async def handle_block_break(self, client_id: str, data: dict):
        """Handle block breaking"""
        q = data.get("q")
        r = data.get("r")
        player_id = data.get("player_id")
        
        update = BlockUpdate(q=q, r=r, block_type=0, player_id=player_id)  # 0 = AIR
        self.block_updates.append(asdict(update))
        
        # Keep only last 1000 updates
        if len(self.block_updates) > 1000:
            self.block_updates = self.block_updates[-1000:]
        
        # Broadcast to all clients
        await self.broadcast_message({
            "type": "block_broken",
            "update": asdict(update)
        })
    
    async def handle_item_drop(self, client_id: str, data: dict):
        """Handle item drop"""
        item_id = data.get("item_id")
        q = data.get("q")
        r = data.get("r")
        block_type = data.get("block_type")
        quantity = data.get("quantity", 1)
        velocity_x = data.get("velocity_x", 0.0)
        velocity_y = data.get("velocity_y", 0.0)
        
        item = ItemDrop(
            id=item_id,
            q=q,
            r=r,
            block_type=block_type,
            quantity=quantity,
            velocity_x=velocity_x,
            velocity_y=velocity_y
        )
        
        self.dropped_items[item_id] = item
        
        # Broadcast to all clients
        await self.broadcast_message({
            "type": "item_dropped",
            "item": asdict(item)
        })
    
    async def handle_item_pickup(self, client_id: str, data: dict):
        """Handle item pickup"""
        item_id = data.get("item_id")
        
        if item_id in self.dropped_items:
            del self.dropped_items[item_id]
            
            # Broadcast to all clients
            await self.broadcast_message({
                "type": "item_picked_up",
                "item_id": item_id
            })
    
    async def remove_player(self, client_id: str):
        """Remove player from server"""
        # Find and remove player
        player_to_remove = None
        for player_id, player in self.players.items():
            # Find player associated with this client
            if client_id in str(player.id):
                player_to_remove = player_id
                break
        
        if player_to_remove:
            del self.players[player_to_remove]
            await self.broadcast_message({
                "type": "player_left",
                "player_id": player_to_remove
            })
            logger.info(f"Player left: {player_to_remove}")
    
    async def broadcast_message(self, message: dict):
        """Broadcast message to all connected clients"""
        if not self.clients:
            return
            
        message_str = json.dumps(message)
        
        # Send to all clients
        for client_id, websocket in self.clients.items():
            try:
                await websocket.send(message_str)
            except Exception as e:
                logger.error(f"Error sending to client {client_id}: {e}")
    
    async def broadcast_to_others(self, exclude_client_id: str, message: dict):
        """Broadcast message to all clients except one"""
        message_str = json.dumps(message)
        
        for client_id, websocket in self.clients.items():
            if client_id != exclude_client_id:
                try:
                    await websocket.send(message_str)
                except Exception as e:
                    logger.error(f"Error sending to client {client_id}: {e}")
    
    async def start(self):
        """Start the server"""
        self.running = True
        logger.info(f"Starting server on {self.host}:{self.port}")
        
        async with websockets.serve(self.handle_client, self.host, self.port):
            logger.info("Server is running...")
            await asyncio.Future()  # Run forever
    
    async def stop(self):
        """Stop the server"""
        self.running = False
        logger.info("Server stopped")

async def main():
    """Main entry point"""
    server = GameServer()
    
    try:
        await server.start()
    except KeyboardInterrupt:
        logger.info("Server interrupted")
        await server.stop()

if __name__ == "__main__":
    asyncio.run(main())